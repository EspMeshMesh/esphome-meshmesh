#pragma once

#include "meshmesh_direct.h"
#include "esphome/components/meshmesh/meshmesh.h"

#include "esphome/core/automation.h"
#include "esphome/core/base_automation.h"
#include "esphome/core/log.h"

#include <cstdint>

namespace esphome {
namespace meshmesh {

template<typename... Ts> class SendAction : public Action<Ts...>, public Parented<MeshMeshDirectComponent> {
  TEMPLATABLE_VALUE(uint32_t, address);
  TEMPLATABLE_VALUE(std::vector<uint8_t>, data);

  void play_complex(Ts... x) override {
    uint32_t address = this->address_.value(x...);
    std::vector<uint8_t> data = this->data_.value(x...);
    ESP_LOGI("meshmesh_direct", "Sending data %d to %d", data.size(), address);
    this->parent_->unicastSendCustom(data.data(), data.size(), address);
  }

  void play(Ts... x) override { /* ignore - see play_complex */
  }

}; // class SendAction

class OnReceiveTrigger : public Trigger<uint32_t , const uint8_t *, uint8_t>, public MeshMeshDirectReceivedPacketHandler {
public:
  explicit OnReceiveTrigger() {}

  bool on_received(uint32_t from, const uint8_t *data, uint8_t size) override {
    this->trigger(from, data, size);
    return true;  // Return true to stop processing other internal handlers
  }
}; // class OnReceiveTrigger

}  // namespace meshmesh
}  // namespace esphome
