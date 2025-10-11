#include "meshmesh_transport.h"
#include "esphome/components/meshmesh/commands.h"

#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/network/util.h"

#include <functional>

#include <espmeshmesh.h>
#include <meshsocket.h>

namespace esphome {
namespace meshmesh {

static const char *const TAG = "meshmesh_transport";
static const uint8_t PACKET_TRANSPORT_PORT = 0xA2;

void MeshmeshTransport::setup() {
  PacketTransport::setup();
  if(!mSocket) openSocket();
}

void MeshmeshTransport::update() {
  this->updated_ = true;
  this->resend_data_ = true;
  PacketTransport::update();
}

void MeshmeshTransport::dump_config() {
  ESP_LOGCONFIG(TAG, "MeshmeshTransport");
  ESP_LOGCONFIG(TAG, "Address: 0x%06X", mTargetAddress);
}

void MeshmeshTransport::set_address(uint32_t address) {
  mTargetAddress.port = PACKET_TRANSPORT_PORT;
  mTargetAddress.address = address == 0 ? espmeshmesh::MeshAddress::broadCastAddress : address;
  if(mSocket != nullptr) {
    delete mSocket;
    openSocket();
  }
}

void MeshmeshTransport::set_repeaters(const std::vector<uint32_t> &repeaters) {
  mTargetAddress.repeaters = repeaters;
  if(mSocket != nullptr) {
    delete mSocket;
    openSocket();
  }
}

void MeshmeshTransport::send_packet(const std::vector<uint8_t> &buf) const {
  if(!mSocket) return;
  int8_t err = mSocket->sendDatagram(buf.data(), buf.size(), mTargetAddress, nullptr);
  if(err != 0) {
    ESP_LOGE(TAG, "Error sending packet to %06X: err:%d", mSocket->getTargetAddress(), err);
  }
}

void MeshmeshTransport::openSocket() {
  mSocket = new espmeshmesh::MeshSocket(PACKET_TRANSPORT_PORT);
  mSocket->recvDatagramCb(std::bind(&MeshmeshTransport::recvDatagram, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  mSocket->sentStatusCb(std::bind(&MeshmeshTransport::sentStatus, this, std::placeholders::_1));
  int8_t err = mSocket->open();
  if(err < 0) {
    ESP_LOGE(TAG, "Error opening socket: %d", err);
  }
}

void MeshmeshTransport::handleFrame(uint8_t *buf, uint16_t len) {
  ESP_LOGD(TAG, "Received packet len %d", len);
  this->process_(std::vector<uint8_t>(buf, buf+len-1));
}

void MeshmeshTransport::recvDatagram(uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from, int16_t rssi) {
  //ESP_LOGD(TAG, "Received datagram from %06X len %d", from, len);
  if(!mTargetAddress.isBroadcast() && from.address != mTargetAddress.address) {
    ESP_LOGE(TAG, "Received datagram from %06X but expected %06X", from, mTargetAddress.address);
    return;
  }
  this->process_(std::vector<uint8_t>(buf, buf+len-1));
}

void MeshmeshTransport::sentStatus(bool success) {
  if(!success) {
    ESP_LOGE(TAG, "Last packet sent status: error");
  } else {
    ESP_LOGD(TAG, "Last packer sent status: success");
  }
}


}  // namespace meshmesh
}  // namespace esphome
