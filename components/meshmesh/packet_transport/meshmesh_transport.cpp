#include "meshmesh_transport.h"
#include "esphome/components/meshmesh/commands.h"

#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/network/util.h"
#include <espmeshmesh.h>

namespace esphome {
namespace meshmesh {

static const char *const TAG = "meshmesh_transport";

void MeshmeshTransport::setup() {
  PacketTransport::setup();
  this->parent_->getNetwork()->addHandleFrameCb(std::bind(&MeshmeshTransport::handleFrame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void MeshmeshTransport::update() {
  this->updated_ = true;
  this->resend_data_ = true;
  PacketTransport::update();
}

void MeshmeshTransport::send_packet(const std::vector<uint8_t> &buf) const {
  if(this->address_ != 0)
    this->parent_->getNetwork()->uniCastSendData(buf.data(), buf.size(), this->address_);
  else
    this->parent_->getNetwork()->broadCastSendData(buf.data(), buf.size());
}

int8_t MeshmeshTransport::handleFrame(uint8_t *buf, uint16_t len, uint32_t from) {
  if(len < 1 || buf[0] != CMD_PACKET_TRANSPORT_REQ) {
    return FRAME_NOT_HANDLED;
  }

  ESP_LOGD(TAG, "Received packet from %d, len %d", from, len);
  this->process_(std::vector<uint8_t>(buf+1, buf+len-2));
  return HANDLE_UART_OK;
}


}  // namespace meshmesh
}  // namespace esphome
