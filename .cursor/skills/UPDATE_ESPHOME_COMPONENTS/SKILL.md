---
name: esphome-compatibility
description: Procedure update all EspHome overridden components to a specific EspHome revision for compatibility with newer EspHome versions
---

# EspHome compatibility update

Detailed instructions for the agent.

## When to use 

- Use this skill when user ask to update overridden EspHome components to a specific EspHome version
- This skill describe the full procedure to update the component
- The overridden components to update are (socket, network, esphome/ota and audio)

# Procedure to update 

## Procedure

Update an EspHome overridden component (network, esphome/ota , socket) is done by
coping the original EspHome component sources (also the python part) and re-apply to 
the sources the changes done to make the MeshMesh network work.

If there is a breaking change and is not possible to apply the patch we need to stop the
update process and handle it by hand.

Every component must be committed separately with a message like "component X ported to esphome 2026.Z", 
the push is done after a human review.

Follow specific instructions for every component:

1. In the **network** and the **esphome/ota** components for instance all MeshMesh changes are 
contained inside a #ifdef USE_MESH_MESH block.

2. In the **socket** component all MeshMesh changes are handle by #ifdef USE_SOCKET_IMPL_MESHMESH_ESP32 
or #ifdef USE_SOCKET_IMPL_MESHMESH_ESP8266 plus the specific meshmesh_socket_impl.* files.

