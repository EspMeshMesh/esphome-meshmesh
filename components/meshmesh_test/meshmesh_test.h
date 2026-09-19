#pragma once

#include "esphome/core/component.h"
#include <meshaddress.h>
#include <string>

namespace espmeshmesh {
  class MeshSocket;
}

namespace esphome {
namespace meshmesh {

class MeshmeshTest : public Component {
  enum State { WAIT_START, BROADCAST1, BROADCAST2, UNICAST1, UNICAST2, DONE };
public:
  void setup() override;
  void loop() override;
  void dump_config() override;
public:
  void set_index(uint16_t index) { mIndex = index; }
public:
  float get_setup_priority() const override { return setup_priority::LATE; }
private:
  void wait_start();
  static const std::string wait_starttitle;
  void broadcast1();
  void broadcast1AsyncRecv(uint8_t *buf, uint16_t len, uint32_t from, int16_t rssi);
  static const std::string broadcast1title;
  void broadcast2();
  static const std::string broadcast2title;
  void unicast1();
  static const std::string unicast1title;
  void unicast2();
  static const std::string unicast2title;
  void unicast2AsyncRecv(const uint8_t *data, uint16_t size, const espmeshmesh::MeshAddress &from, int16_t rssi);
private:
  uint8_t checksum(const uint8_t *buffer, uint16_t size) const;
  int16_t openTestSocket();
  void closeTestResources();
  int16_t sendTestDatagram(const uint8_t *data, uint16_t size, const espmeshmesh::MeshAddress &target);
  int16_t recvTestDatagram();
  espmeshmesh::MeshAddress broadcastTarget() const;
  espmeshmesh::MeshAddress unicastTarget() const;
  uint16_t afterOpenSubState() const;
private:
  uint16_t mIndex{0};
  uint16_t mState{0};
  uint16_t mSubState{0};
  uint32_t mLastTime{0};
  espmeshmesh::MeshSocket *mSocket{nullptr};
  uint8_t *mBuffer{nullptr};
  int16_t mRssi{0};
  espmeshmesh::MeshAddress mFrom;
};


}  // namespace meshmesh
}  // namespace esphome