#pragma once
#include <meshaddress.h>

#include "esphome/core/component.h"
#include "esphome/components/meshmesh/meshmesh.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif


#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

#ifdef USE_LIGHT
#include "esphome/components/light/light_state.h"
#endif

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace espmeshmesh {
  class MeshSocket;
}

namespace esphome {
namespace meshmesh {

/// Handler interface for receiving MeshMeshDirect packets
/// Components should inherit from this class to handle incoming MeshMeshDirect data
class MeshMeshDirectReceivedPacketHandler {
  public:
   /// Called when an MeshMeshDirect packet is received
   /// @param from From address of the received packet
   /// @param data Pointer to the received data payload
   /// @param size Size of the received data in bytes
   /// @return true if the packet was handled, false otherwise
   virtual bool on_received(uint32_t from, const uint8_t *data, uint8_t size) = 0;
 };

class MeshMeshDirectCommandReplyHandler {
public:
  /// Called when a MeshMeshDirect command reply is received
  /// @param from From address of the received packet
  /// @param cmd Command type
  /// @param data Pointer to the received data payload
  /// @param len Size of the received data in bytes
  /// @return true if the packet was handled, false otherwise
  virtual bool onCommandReply(uint32_t from, uint8_t cmd, const uint8_t *data, uint16_t len) = 0;
};

class MeshMeshDirectComponent : public Component {
public:
  typedef enum { UNKNOW = 0, LUX, LAST_SENSOR_TYPE } SensorTypes;
  typedef enum { AllEntities = 0, SensorEntity, BinarySensorEntity, SwitchEntity, LightEntity,TextSensorEntity, LastEntity} EnityType;
public:
  MeshMeshDirectComponent();
  void setup();
  void dump_config() override;
  espmeshmesh::MeshSocket *socket() { return mSocket; }
  // messhmesh: BEFORE_CONNECTION --> meshmesh_direct AFTER_CONNECTION --> Entities: LATE
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }
  // Custom data received handlers
public:
  void register_received_handler(MeshMeshDirectReceivedPacketHandler *handler) { this->mReceivedHandlers.push_back(handler); }
private:
  std::vector<MeshMeshDirectReceivedPacketHandler *> mReceivedHandlers;
// Command reply handlers
public:
  void registerCommandReplyHandler(MeshMeshDirectCommandReplyHandler *handler) { this->mCommandReplyHandlers.push_back(handler); }
private:
  std::vector<MeshMeshDirectCommandReplyHandler *> mCommandReplyHandlers;
public:
  void broadcastSend(const uint8_t cmd, const uint8_t *data, const uint16_t len);
  void broadcastSendCustom(const uint8_t *data, const uint16_t len);
  void unicastSend(const uint8_t cmd, const uint8_t *data, const uint16_t len, const uint32_t addr);
  void unicastSendCustom(const uint8_t *data, const uint16_t len, const uint32_t addr);
public:
#ifdef USE_SWITCH
    void publishRemoteSwitchState(uint32_t addr, uint16_t hash, bool state);
#endif
private:
#ifdef USE_SENSOR
    sensor::Sensor *findSensorByUnit(const std::string &unit);
    sensor::Sensor *findSensor(uint16_t hash);
#endif
#ifdef USE_BINARY_SENSOR
    binary_sensor::BinarySensor *findBinarySensor(uint16_t hash);
#endif
#ifdef USE_LIGHT
    light::LightState *findLightState(uint16_t hash);
#endif
#ifdef USE_TEXT_SENSOR
    text_sensor::TextSensor *findTextSensor(uint16_t hash);
#endif
#ifdef USE_SWITCH
    switch_::Switch *findSwitch(uint16_t hash);
#endif
   private:
    EnityType findEntityTypeByHash(uint16_t hash);
private:
  void handleFrame(const uint8_t *data, uint16_t size, const espmeshmesh::MeshAddress &from, int16_t rssi);
  void handleEntityFrame(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
  void handleEntitiesCountFrame(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
  void handleGetEntityHashFrame(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
  void handleGetEntityStateFrame(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
  void handleSetEntityStateFrame(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
  void handlePublishEntityStateFrame(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
  void handleCustomDataFrame(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
private:
  void handleCommandReply(const uint8_t *data, uint16_t len, const espmeshmesh::MeshAddress &from);
  espmeshmesh::MeshSocket *mSocket{nullptr};
};
}  // namespace meshmesh
}  // namespace esphome
