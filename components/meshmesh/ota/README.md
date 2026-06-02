# MeshMesh OTA

Standalone over-the-air firmware updates over the ESPMeshMesh **datagram** socket on port **0xA5** (165).

This component implements the same byte-level [ESPHome OTA protocol](https://esphome.io/components/ota/) as `ota: platform: esphome` (magic, features, optional SHA256 password, MD5, firmware stream, v1/v2 chunk acknowledgements). It does **not** use TCP or the standard OTA port (3232).

## Configuration

Add `meshmesh.ota` to your external components list, then:

```yaml
external_components:
  - source: github://EspMeshMesh/esphome-meshmesh@main
    components: [meshmesh, meshmesh.ota, network, socket, esphome]

meshmesh:
  # ... meshmesh settings ...

meshmesh_ota:
  id: meshmesh_ota_1
  version: 2
  password: !secret ota_password
```

| Option | Description |
|--------|-------------|
| `version` | OTA protocol version `1` or `2` (default `2`). Version 2 sends `CHUNK_OK` every 8192 bytes. |
| `password` | Optional OTA password (SHA256 auth, same as ESPHome OTA). |

## Coexistence with WiFi OTA

You can still use WiFi `ota: platform: esphome` on the same device. Mesh OTA listens only on mesh port **0xA5**; WiFi OTA uses TCP on port 3232.

## Client requirement

The ESPHome CLI and Dashboard perform OTA over **TCP**. To update a node over the mesh you need a **mesh OTA client** that:

1. Sends the ESPHome OTA byte stream split into unicast datagrams to the target node address on port **0xA5**.
2. Receives response datagrams from the device (same port / return path).

Firmware payloads may span many datagrams; the server reassembles them in an 8 KiB receive buffer.

## Protocol summary

| Step | Direction | Content |
|------|-----------|---------|
| Magic | Client → device | `6C 26 F7 5C 45` |
| Magic ack | Device → client | `OK` + version byte |
| Features | Client → device | 1 byte flags |
| Features ack | Device → client | 1 byte |
| Auth (optional) | Both | SHA256 challenge/response |
| Data | Client → device | Size, MD5, binary image |
| Progress | Device → client | Single-byte status codes |

See also [docs/services_port.md](../../../docs/services_port.md).
