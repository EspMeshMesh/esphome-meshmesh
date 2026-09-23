---
name: MeshMesh OTA test
description: >-
  Use this when testing MeshMesh OTA end-to-end: serial-flash coordinator and
  nodes, start meshmeshgo, then OTA-update a node through the mesh virtual
  address.
---
# MeshMesh OTA test procedure

End-to-end check that a node can be updated over-the-air through the mesh hub (meshmeshgo), not only by USB.

## When to use

- After porting or changing `esphome/ota`, `socket`, `network`, or MeshMesh transport code.
- After bumping ESPHome version for MeshMesh compatibility.
- Whenever you need to verify OTA still works through meshmeshgo.

## Prerequisites

- Hardware on the lab machine that owns the USB serial adapters (typically stefano-MP20).
- Workspace root: `/home/stefano/Sviluppo/Meshmesh`
- ESPHome from `esphome-meshtest` venv (or equivalent), matching the ESPHome version under test.
- `esphome-meshmesh` components at the revision under test.

## Device map

| Role | Serial | YAML |
|------|--------|------|
| Coordinator | `/dev/ttyUSB0` | `esphome-meshtest/test_coord.yaml` |
| Node 1 | `/dev/ttyUSB1` | `esphome-meshtest/test_node_1.yaml` |
| Node 2 | `/dev/ttyUSB2` | `esphome-meshtest/test_node_2.yaml` |
| Node 3 | `/dev/ttyUSB4` | `esphome-meshtest/test_node_3.yaml` |
| Node 4 | `/dev/ttyUSB5` | `esphome-meshtest/test_node_4_8266.yaml` |


Node 2 OTA virtual address: `127.181.110.196`
Node 4 Esp8266 OTA virtual address: `127.117.96.211`

## Procedure

```bash
ESPHOME=/home/stefano/Sviluppo/Meshmesh/esphome-meshtest/venv/bin/esphome
MESHTEST=/home/stefano/Sviluppo/Meshmesh/esphome-meshtest
MESHMESHGO=/home/stefano/Sviluppo/Meshmesh/meshmeshgo
```

### 1. Compile and flash the coordinator

```bash
cd "$MESHTEST"
"$ESPHOME" compile test_coord.yaml
"$ESPHOME" upload test_coord.yaml --device /dev/ttyUSB0
```

### 2. Compile and flash test_node_1

```bash
cd "$MESHTEST"
"$ESPHOME" compile test_node_1.yaml
"$ESPHOME" upload test_node_1.yaml --device /dev/ttyUSB1
```

Optional logs: `timeout 25 "$ESPHOME" logs test_node_1.yaml --device /dev/ttyUSB1`

### 3. Compile and flash test_node_2

```bash
cd "$MESHTEST"
"$ESPHOME" compile test_node_2.yaml
"$ESPHOME" upload test_node_2.yaml --device /dev/ttyUSB2
```

Optional logs: `timeout 25 "$ESPHOME" logs test_node_2.yaml --device /dev/ttyUSB2`

### 4. Launch meshmeshgo

Leave it running for step 5:

```bash
cd "$MESHMESHGO"
go run .
```

Use the lab’s usual serial/config for the coordinator UART. Confirm the hub can see the mesh before OTA.

### 5. OTA-update node 2 through the mesh

```bash
cd "$MESHTEST"
"$ESPHOME" run test_node_2.yaml --device 127.181.110.196
```

Success: OTA completes; node 2 reboots and rejoins the mesh. Watch UART for panic, OTA failure, or `receiving features response: timed out`.

## Notes

- YAML names use underscores: `test_node_1.yaml`, `test_node_2.yaml`.
- Coordinator may have `baud_rate: 0` (no UART logs).
- Do not push or merge unless the user asks.
