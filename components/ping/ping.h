#pragma once
#include <esphome/core/defines.h>

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

#include "esphome/core/component.h"

#include <meshaddress.h>

namespace espmeshmesh {
  class MeshSocket;
}

namespace esphome {
namespace meshmesh {

class PingComponent : public PollingComponent {
  enum class PingPacket { PING=1, PONG=2 };
public:
  PingComponent();

  virtual void setup() override;
  virtual void dump_config() override;
  virtual void loop() override;
  virtual void update() override;

  void set_target_address(uint32_t target_address);
  void set_repeaters(const std::vector<uint32_t> &value);
#ifdef USE_BINARY_SENSOR
  void set_presence_sensor(binary_sensor::BinarySensor *presence_sensor) { mPresenceSensor = presence_sensor; }
#endif
#ifdef USE_SENSOR
  void set_latency_sensor(sensor::Sensor *latency_sensor) { mLatencySensor = latency_sensor; }
#endif
  float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }
private:
  void openSocket();
  void recvDatagram(const uint8_t *data, uint16_t size, const espmeshmesh::MeshAddress &from, int16_t rssi);
private:
  espmeshmesh::MeshSocket *mSocket{nullptr};
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *mPresenceSensor{nullptr};  
#endif
#ifdef USE_SENSOR
  sensor::Sensor *mLatencySensor{nullptr};
#endif
protected:
  espmeshmesh::MeshAddress mTargetAddress;
  uint32_t mLastPingTime = 0;
};

}  // namespace meshmesh
}  // namespace esphome