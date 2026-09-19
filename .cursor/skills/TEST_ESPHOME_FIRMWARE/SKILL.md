---
name: esphome-test-firmware
description: Procedure flash the current esphome and esphome-meshmesh firmware on the test devices and check results.
---

# EspHome compatibility update

Detailed instructions for the agent.

## When to use 

- Use this skill when there is the need to test a new e esphome firmware or a esphome-meshmesh patch.
- This skill describe the full procedure to flash components
- The overridden components to update are (socket, network, esphome/ota and audio)

# Procedure to flash

## Procedure

The config files for test devices are placed in esphome-meshtest folder, the are the following relations
between the serial port names and the config files:

1. Coordinator: /dev/ttyUSB0 --> test_coord.yaml
2. Esp32 Node1: /dev/ttyUSB1 --> test_node1.yaml
3. Esp32 Node2: /dev/ttyUSB2 --> test_node2.yaml
4. Esp32 Node3: /dev/ttyUSB3 --> test_node3.yaml
5. Esp8266 Node4: /dev/ttyUSB3 --> test_node4_8266.yaml

- First compile the coordinator firmware and flash to the coordinator device.
- Next compile de test node1 and flash to corresponding device.
- Monitor the log files of node1 device for errors and critical errors.


