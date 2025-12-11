# [ESPMeshMesh Network](https://www.espmeshmesh.org/) components

ESPMeshMesh is an implementation of a protocol for mesh communication of [ESPHome](https://esphome.io/) nodes that works on ESP8266 and ESP32 based boards and can be integrated with a Home Assistant instance.  The protocol is based on the **802.11B** frame format and is compatible with the radio equipment of chips such as **ESP32** and **ESP8266**.

## Use as external component

The component can be imported in existing configuration as an [external component](https://esphome.io/components/external_components/), in order to make the network work some other bundled components must be overridden. 

```yaml
external_components:
  - source: github://EspMeshMesh/esphome-meshmesh@main
    components: [meshmesh, meshmesh_direct, network, socket, esphome]
```

## Socket component override

The ESPHome socket bundled component must be overridden to make the network compatible with API and OTA components.

```yaml
socket:
  implementation: meshmesh_esp8266
```

* implementation (Required, string): Is the socket implementation compatible with the mesh network architecture. Must be set to **meshmesh_esp8266** or **meshmesh_esp32** value depending on the architecture of the cpu.

## Meshmesh component

This component allows ESPHome to communicate with other ESPHome devices using a mesh protocol based on the 802.11B frame. This network is compatible with both ESP8266 and ESP32 devices.

### Example Configuration

```yaml
# Example configuration 
meshmesh:
  channel: 3
  password: !secret meshmesh_password
```
### Configuration Variables

* **channel** (Required, int): The Wi-Fi channel that ESPMeshmesh will use to send/receive data packets. 
* **password** (Required, string): The AES shared secret used to encrypt all packets on the network.
* **hardware_uart** (Optional, int) default to **0**: UART number used by the base node (coordinator) to communicate with the HUB (meshmeshgo).
* **baud_rate** (Optional, int) default to **460800**: Baud rate of the serial port used to communicate with the HUB.
* **rx_buffer_size** (Optional, int) default to **2048**: Receive buffer size for the serial port used to communicate with the HUB.
* **tx_buffer_size** (Optional, int) default to **4096**: Transmit buffer size for the serial port used to communicate with the HUB.
* **is_coordinator** (Optional, bool) default **false**: Define is this node is a coordinator or not. This will permit the correct initialization of some protocols.
* **use_starpath** (Optional, bool) default **false**: Enable the new experimental protocol that implement the dynamic network.

### Simple example

#### Coordinator node

```yaml
meshmesh:
  baud_rate: 460800
  rx_buffer_size: 2048
  tx_buffer_size: 4096
  password: !secret meshmesh_password
  channel: 3
  is_coordinator: true
  use_starpath: true
```

#### Generic network node

```yaml
meshmesh:
  baud_rate: 0
  rx_buffer_size: 0
  tx_buffer_size: 0
  password: !secret meshmesh_password
  channel: 3
  use_starpath: true
```

## Packet Transport Platform

The [Packet Transport Component](https://esphome.io/components/packet_transport) platform allows ESPHome nodes to directly communicate with each over a communication channel. The MeshMesh implementation of the platform uses MeshMesh as a communication medium. See the [Packet Transport Component](https://esphome.io/components/packet_transport) and [MeshMesh communication](#meshmesh-component) Component for more information.

### Example Configuration

```yaml
# Example configuration entry for packet transport over espmeshmesh.
packet_transport:
  platform: meshmesh
  update_interval: 5s
  address: 0x112233
  repeaters:
    - 0x123456
    - 0x563412

```

### Configuration Variables

* **address** (Required, **server**, **coordinator**, **broadcast** or int): The address for the destination of the transport packets, use keyword **server** will only receive data, use the keyword **coordinator** to send data to the coordinator in a dynamic network, ise **broadcast** to send data to all neighbors. Default is **coordinator**. 
* **repeaters** (Optional, list of int) The sequence of repeaters to use to reach the address.
* All other options from the [Packet Transport Component](https://esphome.io/components/packet_transport/)

### Simple example

Let consider a node with id 0x112233 as provider and a node with id 0xB56EC4 as consumer.

#### Provider node configuration

```yaml
packet_transport:
  - platform: meshmesh
    update_interval: 5s
    address: 0xB56EC4
    sensors:
      - uptime_sensor

sensor:
  - platform: uptime
    name: "Uptime"
    id: uptime_sensor
```

#### Consumer node configuration

```yaml
packet_transport:
  - platform: meshmesh
    update_interval: 5s
    address: 0xC0E5A8
    providers:
      - name: prov-temp-sensor

sensor:
  - platform: packet_transport
    internal: false
    name: Remote Uptime
    provider: prov-temp-sensor
    id: remote_uptime_sensor
    remote_id: uptime_sensor
```

## Meshmesh Direct

```yaml
# Example configuration for meshmesh_direct
meshmesh_direct:
```

Automations:

* **on_receive** (Optional, int) default to **2048**: Receive buffer size for the serial port used to communicate with the HUB.

Actions:

* **meshmesh.send**: This is an action for sending a data packet over the ESPMeshmesh protocol.

Configuration variables:

* **address** (Required, int): Target of the transmission. This can be obtained from the last three bytes of the MAC address.
* **data** (Required, string): Data to transmit to the remote node.

## Meshmesh Direct Switch

Switch component: This is a virtual switch component that can be used to control a remote physical switch present on the remote node.

```yaml
switch:
  - platform: meshmesh_direct
    name: "Switch Name"
    address: 0xAABBCC
    target: 0xDDEE
```

Configuration variables:

* **address** (Required, int): The address for the remote node that contains the switch.
* **target** (Required, int): The hash value of the physical switch on the remote node.
* **id** (Optional, string): Manually specify the ID for code generation. At least one of id and name must be specified.
* **name** (Optional, string): The name of the switch. At least one of id and name must be specified.


## Ping component

The ping component check periodically the connection with another node of the network it provide a presence sensor (binary_sensor) and a latency sensor to trigger actions when the network connectivity change.


Example config #1 passive PONG only device. 

```yaml
ping:
  address: server
```

Example config to ping only the coordinator

```yaml
ping:
```

Example config #2 ping device with address 0xC0E5A8 using devices 0x123456 and 0x563412. The device will reply with a PONG using the reverse path.

```yaml
ping:
  update_interval: 60s
  address: 0xC0E5A8
  repeaters:
    - 0x123456
    - 0x563412

binary_sensor:
  - platform: ping
    presence:
      id: presence

sensor:
  - platform: ping
    latency:
      id: latency
```

Example config #3 ping coordinator device. 

> Require starpath protcol to be enabled.

```yaml
ping:
  update_interval: 60s
  address: coordinator

binary_sensor:
  - platform: ping
    presence:
      id: presence

sensor:
  - platform: ping
    latency:
      id: latency
```

Configuration variables:

* **update_interval**  (Optional, Time): The interval between pings. Defaults to 30s.
* **address** (Optional, **server**, **coordinator** or int): The address for the remote node to ping, use keyword **server** to ping if the device is passive, use the keyword **coordinator** to ping the coordinator in a dynamic network. Default is **coordinator**.
* **repeaters** (Optional, list of int): The sequence of repeaters to use to reach the address.
