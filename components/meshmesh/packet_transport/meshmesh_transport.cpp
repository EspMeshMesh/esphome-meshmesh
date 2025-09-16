#include "meshmesh_transport.h"
#include "esphome/components/meshmesh/commands.h"

#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/network/util.h"

#include <functional>

#include <espmeshmesh.h>
#include <socket.h>

namespace esphome {
namespace meshmesh {

static const char *const TAG = "meshmesh_transport";

void MeshmeshTransport::setup() {
  PacketTransport::setup();
  if(!socket_) {
    ESP_LOGE(TAG, "Socket not set");
    return;
  }
}

void MeshmeshTransport::update() {
  this->updated_ = true;
  this->resend_data_ = true;
  PacketTransport::update();
}

void MeshmeshTransport::set_address(uint32_t address) {
  if(socket_ != nullptr)
    delete socket_;
  socket_ = new espmeshmesh::Socket(0xA2, address);
  //socket_->recvCb(std::bind(&MeshmeshTransport::handleFrame, this, std::placeholders::_1, std::placeholders::_2));
  socket_->recvDatagramCb(std::bind(&MeshmeshTransport::recvDatagram, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void MeshmeshTransport::send_packet(const std::vector<uint8_t> &buf) const {
  if(!socket_) {
    return;
  }
  uint8_t err = socket_->send(buf.data(), buf.size());
  if(err != 0) {
    ESP_LOGE(TAG, "Error sending packet to %06X: err:%d", socket_->getTargetAddress(), err);
  }
}

void MeshmeshTransport::handleFrame(uint8_t *buf, uint16_t len) {
  ESP_LOGD(TAG, "Received packet len %d", len);
  this->process_(std::vector<uint8_t>(buf, buf+len-1));
}

void MeshmeshTransport::recvDatagram(uint8_t *buf, uint16_t len, uint32_t from, int16_t rssi) {
  ESP_LOGD(TAG, "Received datagram from %06X len %d", from, len);
  if(!socket_->isBradcastTarget() && from != socket_->getTargetAddress()) {
    ESP_LOGE(TAG, "Received datagram from %06X but expected %06X", from, socket_->getTargetAddress());
    return;
  }
  this->process_(std::vector<uint8_t>(buf, buf+len-1));
}


}  // namespace meshmesh
}  // namespace esphome
