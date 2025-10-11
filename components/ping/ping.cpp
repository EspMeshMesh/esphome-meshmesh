#include "ping.h"

#include <esphome/core/log.h>

#include <espmeshmesh.h>
#include <meshsocket.h>

#include <functional>
#include <string.h>

namespace esphome {
namespace meshmesh {

static const char *TAG = "ping";
static const uint8_t MESHMESH_PING_PORT = 0xA4;


PingComponent::PingComponent() {
    ESP_LOGI(TAG, "PingSensor constructor");
}

void PingComponent::setup() {
    if(!mSocket) openSocket();
}

void PingComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "PingComponent");
    ESP_LOGCONFIG(TAG, "Target Address: %06X", mTargetAddress);
    ESP_LOGCONFIG(TAG, "Log level: %d", ESPHOME_LOG_LEVEL);
#ifdef USE_BINARY_SENSOR
    ESP_LOGCONFIG(TAG, "Binary sensor: %s", mPresenceSensor!=nullptr ? "true" : "false");
#endif
#ifdef USE_SENSOR
    ESP_LOGCONFIG(TAG, "Sensor: %s", mLatencySensor!=nullptr ? "true" : "false");
#endif
}

void PingComponent::loop() {
    if(mLastPingTime == 0) return;
    uint32_t now = millis();
    if(now - mLastPingTime > 1000) {
        mLastPingTime = 0;
#ifdef USE_BINARY_SENSOR
        if(mPresenceSensor) {
            mPresenceSensor->publish_state(false);
        }
#endif
        ESP_LOGV(TAG, "PingComponent loop timeout");
    }
}

void PingComponent::update() {
    if(mTargetAddress <= 1) return;
    ESP_LOGV(TAG, "PingComponent update send TO %06X", mTargetAddress);
    uint8_t pkt[] = { static_cast<uint8_t>(PingPacket::PING), 'P', 'I', 'N', 'G' };
    mLastPingTime = millis();
    if(mSocket) mSocket->sendDatagram(pkt, 5, mTargetAddress, MESHMESH_PING_PORT, nullptr, nullptr);
}

void PingComponent::openSocket() {
    if(mTargetAddress == 0) return;
    mSocket = new espmeshmesh::MeshSocket(MESHMESH_PING_PORT, mTargetAddress);
    mSocket->recvDatagramCb(std::bind(&PingComponent::recvDatagram, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    int8_t err = mSocket->open();
    if(err < 0) {
      ESP_LOGE(TAG, "Error opening socket: %d", err);
    }
}

void PingComponent::set_target_address(uint32_t target_address) {
    mTargetAddress = target_address;
    ESP_LOGI(TAG, "PingComponent set_target_address: 0x%06X", mTargetAddress);
}

void PingComponent::recvDatagram(uint8_t *buf, uint16_t len, uint32_t from, int16_t rssi) {
    uint32_t now = millis();
    if(buf[0] == static_cast<uint8_t>(PingPacket::PING) && strncmp(reinterpret_cast<char *>(buf), "PING", 4) != 0) {
        uint8_t pkt[] = { static_cast<uint8_t>(PingPacket::PONG), 'P', 'O', 'N', 'G' };
        if(mSocket) mSocket->sendDatagram(pkt, 5, from, MESHMESH_PING_PORT, nullptr, nullptr);
        ESP_LOGV(TAG, "Received a PING packet from %06X", from);
        return;
    }   
    if(buf[0] == static_cast<uint8_t>(PingPacket::PONG) && strncmp(reinterpret_cast<char *>(buf), "PONG", 4) != 0) {
#ifdef USE_BINARY_SENSOR
        if(mPresenceSensor) {
            mPresenceSensor->publish_state(now - mLastPingTime < 1000);
        }
#endif
#ifdef USE_SENSOR
        if(mLatencySensor) {
            mLatencySensor->publish_state(now - mLastPingTime);
        }
#endif
        ESP_LOGV(TAG, "Received a PONG packet from %06X after %d ms", from, now - mLastPingTime);
        mLastPingTime = 0;
        return;
    }    
    ESP_LOGE(TAG, "Received unknown packet len %d", len);
}

}  // namespace meshmesh
}  // namespace esphome