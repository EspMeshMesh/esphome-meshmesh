#pragma once

#include "esphome/core/component.h"

namespace espmeshmesh {
  class MeshSocket;
}

namespace esphome {
namespace meshmesh {

class MeshmeshTest : public Component {
  enum State { BROADCAST1, BROADCAST2, DONE };
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
  static const std::string broadcast1title;
  void broadcast2();
  static const std::string broadcast2title;
private:
  uint16_t mIndex{0};
  uint16_t mState{0};
  uint16_t mSubState{0};
  uint32_t mLastTime{0};
  espmeshmesh::MeshSocket *mSocket{nullptr};
  uint8_t *mBuffer{0};
};


}  // namespace meshmesh
}  // namespace esphome