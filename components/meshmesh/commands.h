#pragma once
namespace esphome::meshmesh
{
    static const uint8_t CMD_FIRMWARE_REQ = 0x02;
    static const uint8_t CMD_FIRMWARE_REP = 0x03;
    static const uint8_t CMD_NODE_TAG_REQ = 0x06;
    static const uint8_t CMD_NODE_TAG_REP = 0x07;
    static const uint8_t CMD_NODE_TAG_SET_REQ = 0x08;
    static const uint8_t CMD_NODE_TAG_SET_REP = 0x09;
    static const uint8_t CMD_CHANNEL_SET_REQ = 0x0C;
    static const uint8_t CMD_CHANNEL_SET_REP = 0x0D;
    static const uint8_t CMD_NODE_CONFIG_REQ = 0x0E;
    static const uint8_t CMD_NODE_CONFIG_REP = 0x0F;
    static const uint8_t CMD_LOG_DEST_REQ = 0x12;
    static const uint8_t CMD_LOG_DEST_REP = 0x13;
    static const uint8_t CMD_LOG_DEST_SET_REQ = 0x14;
    static const uint8_t CMD_LOG_DEST_SET_REP = 0x15;
    static const uint8_t CMD_REBOOT_REQ = 0x18;
    static const uint8_t CMD_REBOOT_REP = 0x19;

    static const uint8_t CMD_GROUPS_REQ = 0x20;
    static const uint8_t CMD_GROUPS_REP = 0x21;
    static const uint8_t CMD_GROUPS_SET_REQ = 0x22;
    static const uint8_t CMD_GROUPS_SET_REP = 0x23;
    // Allocated for MeshMesh Direct
    static const uint8_t CMD_ENTITY_REQ = 0x30;
    static const uint8_t CMD_ENTITY_REP = 0x31;
    // Allocated for Packet Transport
    static const uint8_t CMD_PACKET_TRANSPORT_REQ = 0x32;
    static const uint8_t CMD_PACKET_TRANSPORT_REP = 0x33;

}
