#pragma once
#include "esphome/core/defines.h"
#include "../meshmesh.h"
#include <meshaddress.h>
#ifdef USE_NETWORK
#include "esphome/core/component.h"
#include "esphome/components/packet_transport/packet_transport.h"
#include <vector>

namespace espmeshmesh {
  class MeshSocket;
}

namespace esphome {
namespace meshmesh {

class MeshmeshTransport : public packet_transport::PacketTransport, public Parented<MeshmeshComponent> {
public:
  void setup() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_address(uint32_t address);
  void set_repeaters(const std::vector<uint32_t> &repeaters);

protected:
  void send_packet(const std::vector<uint8_t> &buf) const override;
  size_t get_max_packet_size() override { return 999; }

private:
  void openSocket();
  void handleFrame(uint8_t *buf, uint16_t len);
  void recvDatagram(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from, int16_t rssi);
  void sentStatus(bool success);

private:
  espmeshmesh::MeshSocket *mSocket{nullptr};
  espmeshmesh::MeshAddress mTargetAddress;
};

}  // namespace meshmesh
}  // namespace esphome
#endif
