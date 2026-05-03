# Send messages with meshmesh_direct and polite broadcast protocol

## Introduction

In this example we will use EspMeshMesh send message between a transmitter node and a receiver node using the polite 
broadcast protocol.

## Transmitter node

```yaml
substitutions:
  remote_node_addr: 0xFFFFFFFD

external_components:
  - source: github://EspMeshMesh/esphome-meshmesh@main

esphome:
  name: polite-trnasmitter
  friendly_name: Polite Transimtter

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
  level: INFO

mdns:
  disabled: True

api:
  reboot_timeout: 0s

ota:
  platform: esphome

socket:
  implementation: meshmesh_esp32

meshmesh:
  baud_rate: 0
  rx_buffer_size: 0
  tx_buffer_size: 0
  password: !secret meshmesh_password
  channel: 3
  use_polite_broadcast: True

meshmesh_direct:
  id: meshmesh_direct_id

interval:
  - interval: 5sec
    startup_delay: 20sec
    then:
      - lambda: |-
          const uint8_t data[] = {'H', 'E', 'L', 'L', 'O'};
          id(meshmesh_direct_id)->unicastSendCustom(data, 5, ${remote_node_addr});

sensor:
  - platform: uptime
    name: "Uptime"
    id: uptime_sensor
```


## Receiver node


```yaml
external_components:
  - source: github://EspMeshMesh/esphome-meshmesh@main

esphome:
  name: polite-receiver
  friendly_name: Polite Receiver

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
  level: INFO

mdns:
  disabled: True

socket:
  implementation: meshmesh_esp32

meshmesh:
  baud_rate: 0
  rx_buffer_size: 0
  tx_buffer_size: 0
  password: !secret meshmesh_password
  channel: 3
  use_polite_broadcast: True

api:
  reboot_timeout: 0s

ota:
  platform: esphome

meshmesh_direct:
  id: meshmesh_direct_id
  on_receive:
    then:
      - lambda: |-
            ESP_LOGI("lambda", "on_receive %06X data %s", from,  format_hex_pretty(data, size).c_str());

sensor:
  - platform: uptime
    name: "Uptime"
    id: uptime_sensor
```

## Simple repeater node


```yaml
external_components:
  - source: github://EspMeshMesh/esphome-meshmesh@main

esphome:
  name: polite-repeater
  friendly_name: Polite Repeater

esp32:
  board: esp32dev
  framework:
    type: esp-idf

logger:
  level: INFO

mdns:
  disabled: True

socket:
  implementation: meshmesh_esp32

meshmesh:
  baud_rate: 0
  rx_buffer_size: 0
  tx_buffer_size: 0
  password: !secret meshmesh_password
  channel: 3
  use_polite_broadcast: True

api:
  reboot_timeout: 0s

ota:
  platform: esphome

sensor:
  - platform: uptime
    name: "Uptime"
    id: uptime_sensor
```

