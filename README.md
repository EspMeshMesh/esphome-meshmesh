---
title: "ESPMeshMesh External Components"
description: "Mesh networking external components for ESPHome on ESP8266 and ESP32 devices."
---

ESPMeshMesh provides mesh communication for [ESPHome](https://esphome.io/) nodes on ESP8266 and ESP32 hardware.
It supports direct node-to-node communication and integration with Home Assistant.

## Example Projects

- [Standalone No Hub](examples/01_standalone_no_hub/esp32/remote_switch/README.md)
- [Polite Broadcast](examples/02_polite_broadcast/mesh_direct/README.md)

## External Components Setup

Use the [External Components](/components/external_components/) feature to load this repository in your ESPHome
configuration. This repository also overrides bundled components used by networking and socket layers.

```yaml
external_components:
  - source: github://EspMeshMesh/esphome-meshmesh@main
    components: [meshmesh, meshmesh_direct, network, socket, esphome]
```

## Socket Component Override

Override the bundled `socket` implementation to make API and OTA compatible with ESPMeshMesh.

```yaml
socket:
  implementation: meshmesh_esp8266
```

```yaml
socket:
  implementation: meshmesh_esp32
```

- **implementation** (**Required**, string): Socket backend for the target architecture. Use
  `meshmesh_esp8266` on ESP8266 and `meshmesh_esp32` on ESP32.

## Meshmesh Component

The `meshmesh` component enables packet exchange between ESPHome devices using an IEEE 802.11b frame-based mesh
transport.

### Configuration

```yaml
meshmesh:
  channel: 3
  password: !secret meshmesh_password
```

### Configuration Variables

- **channel** (**Required**, int): Wi-Fi channel used for mesh packet transmission.
- **password** (**Required**, string): Shared AES key used to encrypt mesh packets.
- **hardware_uart** (*Optional*, int): UART index used by the coordinator to connect to the hub. Defaults to `0`.
- **baud_rate** (*Optional*, int): Serial baud rate for hub communication. Defaults to `460800`.
- **rx_buffer_size** (*Optional*, int): UART receive buffer size. Defaults to `2048`.
- **tx_buffer_size** (*Optional*, int): UART transmit buffer size. Defaults to `4096`.
- **is_coordinator** (*Optional*, bool): Marks this node as coordinator. Defaults to `false`.
- **use_starpath** (*Optional*, bool): Enables the experimental dynamic-network protocol. Defaults to `false`.
- **node_type** (*Optional*, enum): One of `backbone`, `coordinator`, or `edge`. Defaults to `backbone`.

### Node Types

- **backbone**: Mains-powered routing node used to extend network coverage.
- **coordinator**: Root node connected to the hub through serial.
- **edge**: Low-power endpoint that does not route packets for other nodes.

### Examples

#### Coordinator

```yaml
meshmesh:
  password: !secret meshmesh_password
  channel: 3
  is_coordinator: true
```

#### Generic Node

```yaml
meshmesh:
  password: !secret meshmesh_password
  channel: 3
```

## Mesh Address Schema

`mesh_address` identifies a destination in the mesh network and can be reused by multiple components.

```yaml
component_name:
  mesh_address:
    protocol: none
    address: 0x112233
    repeaters:
      - 0x123456
      - 0x563412
```

- **protocol** (*Optional*, `none` | `polite_broadcast`): Overrides protocol selection. Defaults to `none`.
- **address** (**Required**, `server` | `coordinator` | `broadcast` | int): Destination node selector.
- **repeaters** (*Optional*, list of int): Ordered repeater path for routed delivery.

Use `polite_broadcast` with a specific `address` to target one node using polite broadcast delivery, or with
`broadcast` to target all nodes.

## Packet Transport Platform

The [Packet Transport](/components/packet_transport/) component can use ESPMeshMesh as a transport backend.
See also the `meshmesh` component in this document.

### Configuration

```yaml
packet_transport:
  - platform: meshmesh
    mesh_address:
      address: 0x112233
```

### Configuration Variables

- **mesh_address** (**Required**, schema): Destination address schema. See [Mesh Address Schema](#mesh-address-schema).
- **All other options** (*Optional*): Inherited from [Packet Transport](/components/packet_transport/).

### Examples

#### Provider Node

```yaml
packet_transport:
  - platform: meshmesh
    mesh_address:
      address: 0xB56EC4
    sensors:
      - uptime_sensor
```

#### Consumer Node

```yaml
packet_transport:
  - platform: meshmesh
    mesh_address:
      address: 0x112233
    providers:
      - name: prov-uptime
```

## Meshmesh Direct

The `meshmesh_direct` component sends and receives arbitrary data frames over ESPMeshMesh.

### Configuration

```yaml
meshmesh_direct:
```

### Automations

- **on_receive** (*Optional*, automation): Triggered when a packet is received.

### Actions

- **meshmesh.send**: Sends a data packet over ESPMeshMesh.

### Action Variables

- **address** (**Required**, int): Destination node address.
- **data** (**Required**, string): Payload to transmit.

## Meshmesh Direct Switch

The `meshmesh_direct` switch platform controls a physical switch exposed by a remote node.

### Configuration

```yaml
switch:
  - platform: meshmesh_direct
    name: "Switch Name"
    address: 0xAABBCC
    target: 0xDDEE
```

### Configuration Variables

- **address** (**Required**, int): Address of the remote node containing the switch.
- **target** (**Required**, int): Hash identifier of the remote switch entity.
- **id** (*Optional*, string): Manually set the generated ID. At least one of `id` and `name` is required.
- **name** (*Optional*, string): Entity display name. At least one of `id` and `name` is required.

## Ping Component

The `ping` component periodically checks reachability of another mesh node and exposes presence and latency data.

### Configurations

#### Passive PONG-Only Device

```yaml
ping:
  address: server
```

#### Ping Coordinator

```yaml
ping:
```

#### Ping Specific Node Through Repeaters

```yaml
ping:
  update_interval: 60s
  address: 0xC0E5A8
  repeaters:
    - 0x123456
    - 0x563412
```

#### Ping Coordinator (Starpath)

> [!IMPORTANT]
> Requires `use_starpath: true` in the `meshmesh` component.

```yaml
ping:
  update_interval: 60s
  address: coordinator
```

### Configuration Variables

- **update_interval** (*Optional*, Time): Interval between pings. Defaults to `30s`.
- **mesh_address** (**Required**, schema): Destination address schema. See [Mesh Address Schema](#mesh-address-schema).
