#include "meshmesh_switch.h"

#include <esphome/core/hal.h>
#include <esphome/core/log.h>

#include "../meshmesh_direct.h"
#include "../commands.h"

#include <packetbuf.h>

namespace esphome {
namespace meshmesh {

static const char *TAG = "meshmesh_direct.switch";

float MeshMeshSwitch::get_setup_priority() const { return setup_priority::LATE; }

struct MeshMeshSwitchState {
  uint32_t address;
  uint16_t hash;
  bool initalState;
};

void MeshMeshSwitch::set_parent(MeshMeshDirectComponent *mmdirect) {
  mMMDirect = mmdirect;
  mMMDirect->registerCommandReplyHandler(this);
}

void MeshMeshSwitch::setup() {
  mRequestUpdate = true;
  mRequestTime = millis();
}

void MeshMeshSwitch::loop() {
  if(mRequestUpdate && millis() - mRequestTime > 500) {
    mLastQueryTime = millis();
    queryRemoteState();
    mRequestUpdate = false;
    mRequestTime = 0;
  }

  if(mLastQueryTime>0 && (millis() - mLastQueryTime) > 1000) {
    mLastQueryTime = 0;
    ESP_LOGW(TAG, "Remote switch '%s' timed out", this->name_.c_str());
  }
}

void MeshMeshSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "Setting up remote switch '%s'...", this->name_.c_str());
  ESP_LOGCONFIG(TAG, " Remote address: 0x%06X", mAddress);
  ESP_LOGCONFIG(TAG, " Remote target: 0x%04X", mHash);
}

void MeshMeshSwitch::queryRemoteState() {
  if(mMMDirect && mMMDirect->socket()) {
    uint8_t buff[3];
    buff[0] = MeshMeshDirectComponent::SwitchEntity;
    espmeshmesh::uint16toBuffer(buff+1, mHash);
    mMMDirect->unicastSend(GET_ENTITY_STATE_REQ, buff, 3, mAddress);
  }
}

bool MeshMeshSwitch::onCommandReply(uint32_t from, uint8_t cmd, const uint8_t *data, uint16_t len) {
  if(cmd == GET_ENTITY_STATE_REP) {
    if(len >= 5) {
      uint8_t value_type = data[0];
      uint16_t hash = espmeshmesh::uint16FromBuffer(data+1);
      if(value_type == 1 && hash == mHash) {
        int16_t value = espmeshmesh::uint16FromBuffer(data+3);
        this->publish_state(value > 0 ? true : false);
        mLastQueryTime = 0;
        return true;
      }
    }
  }
  return false;
}

void MeshMeshSwitch::write_state(bool state) {
  if(mMMDirect->socket()) {
    uint8_t buff[6];
    buff[0] = 1;
    buff[1] = MeshMeshDirectComponent::SwitchEntity;
    espmeshmesh::uint16toBuffer(buff+2, mHash);
    espmeshmesh::uint16toBuffer(buff+4, state ? 10 : 0);
    mMMDirect->unicastSend(SET_ENTITY_STATE_REQ, buff, 6, mAddress);
  }

  this->publish_state(state);
}

}  // namespace meshmesh
}  // namespace esphome
