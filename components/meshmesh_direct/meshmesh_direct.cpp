#include "meshmesh_direct.h"
#include "commands.h"

#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include <espmeshmesh.h>
#include <meshsocket.h>

#include <functional>

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_LIGHT
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_output.h"
#endif

static const char *TAG = "meshmesh_direct";
static const uint8_t MESHMESH_DIRECT_PORT = 0xA6;
namespace esphome {
namespace meshmesh {

MeshMeshDirectComponent::MeshMeshDirectComponent() : Component() {}

void MeshMeshDirectComponent::setup() {
  mSocket = new espmeshmesh::MeshSocket(MESHMESH_DIRECT_PORT);
  mSocket->recvDatagramCb(std::bind(&MeshMeshDirectComponent::handleFrame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  int8_t err = mSocket->open();
  if(err < 0) {
    ESP_LOGE(TAG, "Error opening socket: %d", err);
  }
}

void MeshMeshDirectComponent::broadcastSend(const uint8_t cmd, const uint8_t *data, const uint16_t len) {
  uint8_t *buff = new uint8_t[len+2];
  buff[0] = CMD_ENTITY_REQ;
  buff[1] = cmd;
  memcpy(buff+2, data, len);
  mSocket->sendDatagram(buff, len+2, espmeshmesh::MeshAddress(MESHMESH_DIRECT_PORT, espmeshmesh::MeshAddress::broadCastAddress), nullptr);
  delete buff;
}

void MeshMeshDirectComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Setting up MeshMeshDirectComponent");
  #ifdef USE_SENSOR
  for (auto sensor : App.get_sensors()) {
    ESP_LOGCONFIG(TAG, "Found sensor %s with hash %08X", sensor->get_name().c_str(), sensor->get_object_id_hash());
  }
#endif
#ifdef USE_BINARY_SENSOR
  for (auto sensor : App.get_binary_sensors()) {
    ESP_LOGCONFIG(TAG, "Found sensor %s with hash %08X", sensor->get_name().c_str(), sensor->get_object_id_hash());
  }
#endif
#ifdef USE_SWITCH
  for (auto switch_ : App.get_switches()) {
    ESP_LOGCONFIG(TAG, "Found switch %s with hash %08X", switch_->get_name().c_str(), switch_->get_object_id_hash());
  }
#endif
}

void MeshMeshDirectComponent::broadcastSendCustom(const uint8_t *data, const uint16_t len) {
  broadcastSend(CUSTOM_DATA_REQ, data, len);
}

void MeshMeshDirectComponent::unicastSend(const uint8_t cmd, const uint8_t *data, const uint16_t len, const uint32_t addr) {
  uint8_t *buff = new uint8_t[len+2];
  buff[0] = CMD_ENTITY_REQ;
  buff[1] = cmd;
  memcpy(buff+2, data, len);
  mSocket->sendDatagram(buff, len+2, espmeshmesh::MeshAddress(MESHMESH_DIRECT_PORT, addr), nullptr);
  delete buff;
}

void MeshMeshDirectComponent::unicastSendCustom(const uint8_t *data, const uint16_t len, const uint32_t addr) {
  unicastSend(CUSTOM_DATA_REQ, data, len, addr);
}

void MeshMeshDirectComponent::handleFrame(const uint8_t *data, uint16_t size, const espmeshmesh::MeshAddress &from, int16_t rssi) {
  if(size < 1 || data[0] != CMD_ENTITY_REQ) {
    return;
  }

  handleEntityFrame(data+1, size-1, from);
}

void MeshMeshDirectComponent::handleEntityFrame(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
    if(len < 1) return;

    if (buf[0] & 0x01) {
      handleCommandReply(buf, len, from);
    }

    switch (buf[0]) {
      case ENTITIES_COUNT_REQ:
        handleEntitiesCountFrame(buf, len, from);
      break;

    case GET_ENTITY_HASH_REQ:
      handleGetEntityHashFrame(buf, len, from);
      break;

    case GET_ENTITY_STATE_REQ:
      handleGetEntityStateFrame(buf, len, from);
      break;

    case SET_ENTITY_STATE_REQ:
      handleSetEntityStateFrame(buf, len, from);
      break;

    case PUB_ENTITY_STATE_REQ:
      handlePublishEntityStateFrame(buf, len, from);
      break;

    case CUSTOM_DATA_REQ:
      handleCustomDataFrame(buf, len, from);
      break;

    default:
      break;
    }
}

void MeshMeshDirectComponent::handleEntitiesCountFrame(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
  if (len == 1) {
    uint8_t rep[LastEntity + 2];
    rep[0] = CMD_ENTITY_REQ;
    rep[1] = ENTITIES_COUNT_REP;
    uint8_t *buf = rep + 2;
    memset(buf, 0, LastEntity);

#ifdef USE_SENSOR
    buf[SensorEntity] = (uint8_t) App.get_sensors().size();
    buf[AllEntities] += buf[SensorEntity];
#endif

#ifdef USE_BINARY_SENSOR
    buf[BinarySensorEntity] = (uint8_t) App.get_binary_sensors().size();
    buf[AllEntities] += buf[BinarySensorEntity];
#endif

#ifdef USE_SWITCH
    buf[SwitchEntity] = (uint8_t) App.get_switches().size();
    buf[AllEntities] += buf[SwitchEntity];
#endif

#ifdef USE_LIGHT
    buf[LightEntity] = (uint8_t) App.get_lights().size();
    buf[AllEntities] += buf[LightEntity];
#endif
#ifdef USE_TEXT_SENSOR
    buf[TextSensorEntity] = (uint8_t) App.get_text_sensors().size();
    buf[AllEntities] += buf[TextSensorEntity];
#endif
    mSocket->sendDatagram(rep, LastEntity + 2, from, nullptr);
  }
}

void MeshMeshDirectComponent::handleGetEntityHashFrame(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
  if (len == 3) {
    uint8_t service = buf[1];
    uint8_t index = buf[2];
    uint16_t hash = 0;
    std::string info;
    bool hashfound = false;

    switch (service) {
      case SensorEntity:
#ifdef USE_SENSOR
        if (index < App.get_sensors().size()) {
          auto sensor = App.get_sensors()[index];
          hash = sensor->get_object_id_hash() & 0xFFFF;
          info = sensor->get_name() + "," + sensor->get_name() + "," + sensor->get_unit_of_measurement_ref();
          hashfound = true;
        }
#endif
        break;
      case BinarySensorEntity:
#ifdef USE_BINARY_SENSOR
        if (index < App.get_binary_sensors().size()) {
          auto binary = App.get_binary_sensors()[index];
          hash = binary->get_object_id_hash() & 0xFFFF;
          info = binary->get_name() + "," + binary->get_object_id();
          hashfound = true;
        }
#endif
        break;
      case SwitchEntity:
#ifdef USE_SWITCH
        if (index < App.get_switches().size()) {
          auto switch_ = App.get_switches()[index];
          hash = switch_->get_object_id_hash() & 0xFFFF;
          info = switch_->get_name();
          hashfound = true;
        }
#endif
        break;
      case LightEntity:
#ifdef USE_LIGHT
        if (index < App.get_lights().size()) {
          auto light = App.get_lights()[index];
          hash = light->get_object_id_hash() & 0xFFFF;
          info = light->get_name();
          hashfound = true;
        }
#endif
        break;
      case TextSensorEntity:
#ifdef USE_TEXT_SENSOR
        if (index < App.get_text_sensors().size()) {
          auto texts = App.get_text_sensors()[index];
          hash = texts->get_object_id_hash() & 0xFFFF;
          info = texts->get_name();
          hashfound = true;
        }
#endif
        break;
      case AllEntities:
      case LastEntity:
        break;
    }

    if (hashfound) {
      auto rep = new uint8_t[info.length() + 4];
      rep[0] = CMD_ENTITY_REQ;
      rep[1] = GET_ENTITY_HASH_REP;
      espmeshmesh::uint16toBuffer(rep + 2, hash);
      memcpy(rep + 4, info.c_str(), info.length());
      mSocket->sendDatagram(rep, 4 + info.length(), from, nullptr);
      delete rep;
    } else {
      uint8_t rep[6];
      rep[0] = CMD_ENTITY_REQ;
      rep[1] = GET_ENTITY_HASH_REP;
      rep[2] = 0;
      rep[3] = 0;
      rep[4] = 'E';
      rep[5] = '!';
      mSocket->sendDatagram(rep, 6, from, nullptr);
    }
  }
}

void MeshMeshDirectComponent::handleGetEntityStateFrame(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
  if (len == 4) {
    EnityType type = (EnityType) buf[1];
    uint16_t hash = espmeshmesh::uint16FromBuffer(buf + 2);
    ESP_LOGD(TAG, "GET_ENTITY_STATE_REQ %04X hash %d type", hash, type);
    int16_t value = 0;
    std::string value_str;
    uint8_t value_type = 0;

    switch (type) {
      case SensorEntity: {
#ifdef USE_SENSOR
        sensor::Sensor *sensor = findSensor(hash);
        if(sensor) {
          value = (int16_t) (sensor->state * 10.0);
          value_type = 1;
        }
#endif
      } break;
      case BinarySensorEntity: {
#ifdef USE_BINARY_SENSOR
        binary_sensor::BinarySensor *binary = findBinarySensor(hash);
        if(binary) {
          value = binary->state ? 10 : 0;
          value_type = 1;
        }
#endif
      } break;
      case SwitchEntity: {
#ifdef USE_SWITCH
        auto switch_ = findSwitch(hash);
        if(switch_) {
          value = switch_->state ? 1 : 0;
          value_type = 1;
        }
#endif
      } break;
      case LightEntity: {
#ifdef USE_LIGHT
        light::LightState *state = findLightState(hash);
        if(state) {
          if (state->current_values.get_state() == 0)
            value = 0;
          else
            value = (uint16_t) (state->current_values.get_brightness() * 1024.0);
          value_type = 1;
        }
#endif
      } break;
      case TextSensorEntity: {
#ifdef USE_TEXT_SENSOR
        text_sensor::TextSensor *texts = findTextSensor(hash);
        if(texts) {
          value_str = texts->state;
          value_type = 2;
        }
#endif
      } break;
      case AllEntities:
      case LastEntity:
        break;
    }

    if (value_type == 1) {
      uint8_t rep[7];
      rep[0] = CMD_ENTITY_REQ;
      rep[1] = GET_ENTITY_STATE_REP;
      rep[2] = value_type;
      espmeshmesh::uint16toBuffer(rep + 3, hash);
      espmeshmesh::uint16toBuffer(rep + 5, value);
      mSocket->sendDatagram(rep, 7, from, nullptr);
      return;
    } else if (value_type == 2) {
      uint16_t rep_size = 5 + value_str.length();
      auto *rep = new uint8_t[rep_size];
      rep[0] = CMD_ENTITY_REQ;
      rep[1] = GET_ENTITY_STATE_REP;
      rep[2] = value_type;
      espmeshmesh::uint16toBuffer(rep + 3, hash);
      memcpy(rep + 5, value_str.data(), value_str.length());
      mSocket->sendDatagram(rep, rep_size, from, nullptr);
      return;
    } else {
      ESP_LOGE(TAG, "handleGetEntityStateFrame: Unknown entity with hash %04X", hash);
    }
  }
  return;
}

void MeshMeshDirectComponent::handleSetEntityStateFrame(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
  if (len >= 7) {
    uint8_t value_type = buf[1];
    EnityType entity_type = (EnityType) buf[2];
    uint16_t hash = espmeshmesh::uint16FromBuffer(buf + 3);
    ESP_LOGD(TAG, "SET_ENTITY_STATE_REQ vty %d hash %04X type", value_type, hash, entity_type);
    int16_t value = 0;
    bool valuefound = false;

    switch (entity_type) {
      case SwitchEntity: {
#ifdef USE_SWITCH
        auto switch_ = findSwitch(hash);
        if(switch_ && value_type == 1) {
          value = espmeshmesh::uint16FromBuffer(buf + 5);
          if(value > 0)
            switch_->turn_on();
          else
            switch_->turn_off();
          valuefound = true;
        }
#endif
      } break;
      case LightEntity: {
#ifdef USE_LIGHT
        light::LightState *state = findLightState(hash);
        if(state && value_type == 1) {
          value = espmeshmesh::uint16FromBuffer(buf + 5);
          auto call = state->make_call();
          call.set_state(value > 0 ? 1 : 0);
          call.set_brightness((float) value / 1024.0f);
          call.perform();
          valuefound = true;
        }
#endif
      } break;
      case SensorEntity:
      case BinarySensorEntity:
      case AllEntities:
      case TextSensorEntity:
      case LastEntity:
        break;
    }

    if (valuefound) {
      uint8_t rep[2];
      rep[0] = CMD_ENTITY_REQ;
      rep[1] = GET_ENTITY_STATE_REP;
      mSocket->sendDatagram(rep, 2, from, nullptr);
      return;
    } else {
      ESP_LOGE(TAG, "handleSetEntityStateFrame: Unknown entity with hash %04X", hash);
    }
  }
  return;
}

void MeshMeshDirectComponent::handlePublishEntityStateFrame(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
  if (len == 6) {
    EnityType type = (EnityType) buf[1];
    uint16_t hash = espmeshmesh::uint16FromBuffer(buf + 2);
    uint16_t value = espmeshmesh::uint16FromBuffer(buf + 4);
    ESP_LOGD(TAG, "PUB_ENTITY_STATE_REQ type %d hash %04X value %d", type, hash, value);

    switch (type) {
      case LightEntity: {
#ifdef USE_LIGHT
        // TODO: Implement light publish
#endif
      } break;
      case SwitchEntity: {
#ifdef USE_SWITCH
        auto *state = findSwitch(hash);
        if (state != nullptr) {
          state->publish_state(value > 0 ? true : false);
        }
#endif
      } break;
      case SensorEntity:
      case BinarySensorEntity:
      case TextSensorEntity:
        break;
      case AllEntities:
      case LastEntity:
        break;
    }

    uint8_t rep[2];
    rep[0] = CMD_ENTITY_REQ;
    rep[1] = PUB_ENTITY_STATE_REP;
    mSocket->sendDatagram(rep, 2, from, nullptr);
    return;
  }
  return;
}

void MeshMeshDirectComponent::handleCustomDataFrame(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
  if (len > 1) {
    const uint8_t *data = buf + 1;
    for (auto handler : mReceivedHandlers) {
      handler->on_received(from.address, data, len-1);
    }
    return;
  }
  return;
}

void MeshMeshDirectComponent::handleCommandReply(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from) {
  if (len >= 1) {
    uint8_t cmd = buf[0];
    for (auto handler : mCommandReplyHandlers) {
      if(handler->onCommandReply(from.address, cmd, buf+1, len-1)) {
        return;
      }
    }
  }
  return;
}

#ifdef USE_SENSOR
sensor::Sensor *MeshMeshDirectComponent::findSensorByUnit(const std::string &unit) {
  sensor::Sensor *result = nullptr;
  auto sensors = App.get_sensors();
  for (auto sensor : sensors) {
    result = sensor;
    if (unit == sensor->get_unit_of_measurement_ref()) {
      result = sensor;
      break;
    }
  }
  return result;
}

sensor::Sensor *MeshMeshDirectComponent::findSensor(uint16_t hash) {
  sensor::Sensor *result = nullptr;
  auto sensors = App.get_sensors();
  for (auto sensor : sensors) {
    result = sensor;
    if (hash == (sensor->get_object_id_hash() & 0xFFFF)) {
      result = sensor;
      break;
    }
  }
  return result;
}
#endif

#ifdef USE_BINARY_SENSOR
binary_sensor::BinarySensor *MeshMeshDirectComponent::findBinarySensor(uint16_t hash) {
  binary_sensor::BinarySensor *result = nullptr;
  auto sensors = App.get_binary_sensors();
  for (auto sensor : sensors) {
    if (hash == (sensor->get_object_id_hash() & 0xFFFF)) {
      result = sensor;
      break;
    }
  }
  return result;
}
#endif

#ifdef USE_LIGHT
light::LightState *MeshMeshDirectComponent::findLightState(uint16_t hash) {
  light::LightState *result = nullptr;
  auto lights = App.get_lights();
  for (auto light : lights) {
    if (hash == (light->get_object_id_hash() & 0xFFFF)) {
      result = light;
      break;
    }
  }
  return result;
}
#endif

#ifdef USE_TEXT_SENSOR
text_sensor::TextSensor *MeshMeshDirectComponent::findTextSensor(uint16_t hash) {
  text_sensor::TextSensor *result = nullptr;
  auto sensors = App.get_text_sensors();
  for (auto sensor : sensors) {
    if (hash == (sensor->get_object_id_hash() & 0xFFFF)) {
      result = sensor;
      break;
    }
  }
  return result;
}
#endif

#ifdef USE_SWITCH
switch_::Switch *MeshMeshDirectComponent::findSwitch(uint16_t hash) {
  switch_::Switch *result = nullptr;
  auto switches = App.get_switches();
  for (auto switch_ : switches) {
    if (hash == (switch_->get_object_id_hash() & 0xFFFF)) {
      result = switch_;
      break;
    }
  }
  return result;
}

void MeshMeshDirectComponent::publishRemoteSwitchState(uint32_t addr, uint16_t hash, bool state) {
  uint8_t buff[7];
  buff[0] = CMD_ENTITY_REQ;
  buff[1] = PUB_ENTITY_STATE_REQ;
  buff[2] = SwitchEntity;
  espmeshmesh::uint16toBuffer(buff+3, hash);
  espmeshmesh::uint16toBuffer(buff+5, state ? 10 : 0);
  mSocket->sendDatagram(buff, 7, espmeshmesh::MeshAddress(MESHMESH_DIRECT_PORT, addr), nullptr);
}
#endif

MeshMeshDirectComponent::EnityType MeshMeshDirectComponent::findEntityTypeByHash(uint16_t hash) {
#ifdef USE_SENSOR
  if (findSensor(hash) != nullptr)
    return SensorEntity;
#endif
#ifdef USE_BINARY_SENSOR
  if (findBinarySensor(hash) != nullptr)
    return BinarySensorEntity;
#endif
#ifdef USE_SWITCH
    // if(findSwitch(hash) != nullptr) return SwitchEntity;
#endif
#ifdef USE_LIGHT
  if (findLightState(hash) != nullptr)
    return LightEntity;
#endif
  return AllEntities;
}


}  // namespace meshmesh
}  // namespace esphome
