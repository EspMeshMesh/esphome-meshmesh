#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../../meshmesh_direct/meshmesh_direct.h"

namespace esphome {
namespace meshmesh {

class MeshMeshDirectComponent;
class MeshMeshSwitch : public switch_::Switch, public Component, public MeshMeshDirectCommandReplyHandler {
public:
  void set_target(uint16_t hash, uint32_t address) { mHash = hash; mAddress = address; }
  void set_parent(MeshMeshDirectComponent *mmdirect);
  float get_setup_priority() const override;
  void setup() override;
  void loop() override;
  void dump_config() override;
public:
  void queryRemoteState();
  bool onCommandReply(uint32_t from, uint8_t cmd, const uint8_t *data, uint16_t len) override;
protected:
  void write_state(bool state) override;
private:
  bool mRequestUpdate{false};
  uint32_t mRequestTime{0};
  uint32_t mLastQueryTime{0};
private:
  uint16_t mHash;
  uint32_t mAddress;
  MeshMeshDirectComponent *mMMDirect{0};
};

}  // namespace gpio
}  // namespace esphome
