# ConnectedPath 2-Hop OTA Bug Analysis

## Summary

This document analyzes a bug in the ConnectedPath protocol that causes 2-hop OTA updates to stall after the first ~8KB block. The root cause is incorrect duplicate packet detection in the `espmeshmesh` library that discards legitimate out-of-order packets as duplicates.

## Observed Symptoms

From lab testing (September 2026):

- **1-hop OTA** (coordinator → node1): Works correctly, completes in ~145 seconds
- **2-hop OTA** (coordinator → node1 → node2): Stalls after first 8KB block (~0.7% progress)
- Node2 logs show:
  - Progress reaches 0.1% then 0.7%
  - `ConnectedPath duplicated packet received from <relay>:…`
  - `No data received for 105000 ms`
  - OTA error `0xFF` / "Unknown error from ESP"

## Technical Analysis

### Data Flow Architecture

```
Hub (meshmeshgo)
    │ TCP socket (ESPHome CLI/Dashboard)
    ▼
ConnectionPathBridge
    │ Serial/UART (ConnectedPath protocol)
    ▼
Coordinator (ESP32/ESP8266)
    │ Radio (802.11 raw frames)
    ▼
Node1 (Relay)
    │ Radio (802.11 raw frames)
    ▼
Node2 (OTA Target)
    │ MeshSocket MM_SOCK_STREAM → ESPHomeOTAComponent
```

### ConnectedPath Data Transmission

1. Hub sends data via `ConnPathConnection.SendData()` in 512-byte chunks
2. For ESP32, there's **NO sleep** between chunks (fire-and-forget)
3. Coordinator receives via UART, creates radio packet with new sequence number
4. Relay (Node1) receives, creates NEW radio packet with ITS OWN sequence number
5. Destination (Node2) receives and delivers to application

Each radio hop has its own sequence number space. The relay doesn't pass through the original seqno - it generates a fresh one for its outgoing packet.

### Retransmission Mechanism

In `connectedpath.cpp::radioPacketSent()`:

```cpp
void ConnectedPath::radioPacketSent(uint8_t status, RadioPacket *pkt) {
  if (status) {  // Non-zero = transmission error
    if ((header->flags & CONNPATH_FLAG_RETRANSMIT_MASK) < CONNPATH_MAX_RETRANSMISSIONS) {
      // Create copy with incremented retry counter, SAME seqno
      sendRawRadioPacket(newpkt);  // Goes to back of queue!
    }
  }
}
```

**Critical issue**: When retransmitting, the packet goes to the **back of the packet queue**. Meanwhile, new incoming data packets are also being queued. This causes out-of-order delivery.

### The Bug: Duplicate Detection

In `recvdups.cpp::checkDuplicateTable()`:

```cpp
uint16_t stored = mDuplicates[foundrow].seqno;
if(stored == seqno) {
    return true;  // Exact duplicate - correct
}
if ((uint16_t)(stored - seqno) < 5) {
    return true;  // seqno behind stored - INCORRECT ASSUMPTION!
}
```

The check `(stored - seqno) < 5` assumes that if a sequence number is "behind" the last seen one, it must be a duplicate. **This is wrong when packets arrive out of order.**

### Failure Scenario

1. Coordinator sends packet with seqno=1 to Node1
2. Node1 receives, forwards to Node2 with Node1's seqno=1
3. Transmission to Node2 **fails** (radio collision, interference)
4. Node1 queues retry of seqno=1 packet
5. Meanwhile, Coordinator sends packet with seqno=2
6. Node1 receives, forwards to Node2 with Node1's seqno=2 (queued AHEAD of retry)
7. Node2 receives seqno=2 first, stores `maxSeqno=2`
8. Node2 receives seqno=1 (the retry that was queued behind)
9. Check: `(stored - seqno) = (2 - 1) = 1 < 5` → **Incorrectly marked as DUPLICATE**
10. Legitimate data packet is dropped!

### Why 2-Hop is Worse Than 1-Hop

For 2-hop paths:
- **Two radio links** where packet loss can occur
- **Two chances** for retransmission reordering
- The relay (Node1) is continuously receiving AND transmitting
- Higher traffic volume = more queue contention
- Probability of out-of-order delivery compounds

For 1-hop, only one radio link exists with lower contention.

## The Fix

The fix replaces the simple `seqno` tracking with a 32-bit bitmap that tracks exactly which sequence numbers have been seen:

### Modified Data Structure (`recvdups.h`):

```cpp
struct RecvDupPacket {
    uint32_t address;
    uint32_t time;
    uint32_t seenBitmap;  // Bitmap tracking which of the last 32 seqnos have been seen
    uint16_t handle;
    uint16_t maxSeqno;    // Highest sequence number seen
};
```

### Fixed Algorithm (`recvdups.cpp`):

```cpp
int16_t delta = (int16_t)(seqno - maxSeqno);

if (delta == 0) {
    return true;  // Exact duplicate
}

if (delta > 0) {
    // New seqno ahead - shift bitmap, accept
    bitmap = (delta >= 32) ? 1 : ((bitmap << delta) | 1);
    maxSeqno = seqno;
    return false;
}

// delta < 0: seqno behind maxSeqno (out-of-order)
int offset = -delta;
if (offset >= 32) {
    return true;  // Too old, treat as stale
}

uint32_t mask = 1U << offset;
if (bitmap & mask) {
    return true;  // Already seen this exact seqno
}

// First time seeing this seqno (late arrival, not duplicate)
bitmap |= mask;
return false;
```

This correctly:
- Accepts new sequence numbers ahead of current max
- Accepts out-of-order arrivals within a 32-packet window
- Rejects exact duplicates (actual radio retransmits of already-processed packets)
- Rejects very old packets (>32 behind current max) as stale

## Validation

To validate the fix:

1. **1-hop OTA must still work** - No regression
2. **2-hop OTA should progress past 8KB** - Main symptom resolved
3. **Duplicate count logging** should show fewer false positives
4. **OTA should complete** on 2-hop paths (may still be slow due to hop latency)

## Files Changed

The fix is in the `espmeshmesh` library (separate repository):

- `src/recvdups.h` - Added `seenBitmap` field, renamed `seqno` to `maxSeqno`
- `src/recvdups.cpp` - Replaced simple comparison with bitmap-based tracking

See `connpath-dup-detection-fix.patch` in this directory for the complete patch.

## Related Components

- `espmeshmesh` library (`src/connectedpath.cpp`) - Manages connections, not changed
- `meshmeshgo` hub (`meshmesh/otaconnection.go`) - No change needed (hub-side pacing won't fix the device-side dup detection)
- `esphome-meshmesh` (`components/esphome/ota/ota_esphome.cpp`) - Not changed, receives data through socket abstraction
