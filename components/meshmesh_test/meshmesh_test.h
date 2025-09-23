#pragma once

#include "esphome/core/component.h"

namespace espmeshmesh {
  class MeshSocket;
}

namespace esphome {
namespace meshmesh {

class MeshmeshTest : public Component {
  enum State { BROADCAST1, BROADCAST2, UNICAST1, UNICAST2, DONE };
public:
  void setup() override;
  void loop() override;
  void dump_config() override;
public:
  void set_index(uint16_t index) { mIndex = index; }
public:
  float get_setup_priority() const override { return setup_priority::LATE; }
private:
  void broadcast1();
  void broadcast1AsyncRecv(uint8_t *buf, uint16_t len, uint32_t from, int16_t rssi);
  static const std::string broadcast1title;
  void broadcast2();
  static const std::string broadcast2title;
  void unicast1();
  static const std::string unicast1title;
  void unicast2();
  static const std::string unicast2title;
private:
  uint8_t checksum(const uint8_t *buffer, uint16_t size) const;
private:
  uint16_t mIndex{0};
  uint16_t mState{0};
  uint16_t mSubState{0};
  uint32_t mLastTime{0};
  espmeshmesh::MeshSocket *mSocket{nullptr};
  uint8_t *mBuffer{0};
  uint32_t mFrom{0};
};


}  // namespace meshmesh
}  // namespace esphome