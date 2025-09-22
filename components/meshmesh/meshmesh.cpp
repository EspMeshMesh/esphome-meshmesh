#include "meshmesh.h"
#include "commands.h"
#include <packetbuf.h>
#include <discovery.h>
#include <espmeshmesh.h>

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include "esphome/core/application.h"
#include "esphome/core/version.h"

#define MAX_CHANNEL 13

static const char *TAG = "meshmesh";
static const uint32_t REBOOT_DELAY_TIME = 250;

#ifdef USE_ESP8266
void IRAM_ATTR HOT __wrap_ppEnqueueRxq(void *a) {
	// 4 is the only spot that contained the packets. Discovered by trial and error printing the data
    if(espmeshmesh::PacketBuf::singleton) espmeshmesh::PacketBuf::singleton->rawRecv((espmeshmesh::RxPacket *)(((void **)a)[4]));
	__real_ppEnqueueRxq(a);
}
#endif

namespace esphome::meshmesh {


MeshmeshComponent::MeshmeshComponent(int baud_rate, int tx_buffer, int rx_buffer) {
  this->mesh_ = new espmeshmesh::EspMeshMesh(baud_rate, tx_buffer, rx_buffer);
  this->mesh_->setLogCb(logPrintfCb);
}

void MeshmeshComponent::set_password(const char *password) {
  this->mesh_->setAesPassword(password);
}

void MeshmeshComponent::default_preferences_() {
  // Default preferences
  memset(this->preferences_.devicetag, 0, 32);
  this->preferences_.channel = UINT8_MAX;
  this->preferences_.txPower = UINT8_MAX;
  this->preferences_.flags = 0;
  this->preferences_.log_destination = 0;
  this->preferences_.groups = 0;
#ifdef USE_BONDING_MODE
  // The bonding will permit this node to receive frames only from the bonded node.
  // * 0x0: bonding is disabled,
  // * UINT32_MAX: node not bondend,
  // * otherwise: the node id of the bonded node
  this->preferences_.bonded_node = UINT32_MAX;
#endif
}

void MeshmeshComponent::pre_setup_preferences_() {
  this->default_preferences_();
  this->preferences_object = global_preferences->make_preference<MeshmeshSettings>(fnv1_hash("MeshmeshComponent"), true);
  if (!this->preferences_object.load(&this->preferences_)) {
    ESP_LOGE(TAG, "Can't read prederences from flash");
  }
}


void MeshmeshComponent::pre_setup() {
  this->pre_setup_preferences_();
  this->mesh_->pre_setup();
}

void MeshmeshComponent::setup() {
  espmeshmesh::EspMeshMeshSetupConfig config = {
    .hostname = App.get_name().c_str(),
    .channel = this->preferences_.channel == UINT8_MAX ? this->config_channel_ : this->preferences_.channel,
    .txPower = this->preferences_.txPower,
  };
  this->mesh_->setup(&config);
  this->mesh_->addHandleFrameCb(std::bind(&MeshmeshComponent::handle_frame_, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void MeshmeshComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Meshmesh");
#ifdef USE_ESP32
  ESP_LOGCONFIG(TAG, "Sys cip ID: %08lX", espmeshmesh::Discovery::chipId());
#else
  ESP_LOGCONFIG(TAG, "Sys cip ID: %08X", system_get_chip_id());
  ESP_LOGCONFIG(TAG, "Curr. Channel: %d Saved Channel: %d", wifi_get_channel(), this->preferences_.channel);
#endif
}

void MeshmeshComponent::loop() {
  this->mesh_->loop();

  if (this->reboot_requested_) {
    if (millis() - this->reboot_requested_time_ > REBOOT_DELAY_TIME+) {
      App.reboot();
    }
  }
}

int8_t MeshmeshComponent::handle_frame_(uint8_t *data, uint16_t size, uint32_t from) {
  //ESP_LOGD(TAG, "handleFrame: %d, size: %d, from: %d", data[0], size, from);
  switch (data[0]) {
    case CMD_NODE_TAG_REQ:
      if (size == 1) {
        uint8_t reply[33] = {0};
        reply[0] = CMD_NODE_TAG_REP;
        memcpy(reply + 1, this->preferences_.devicetag, 32);
        this->mesh_->commandReply(reply, 33);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_NODE_TAG_SET_REQ:
      if (size > 1) {
        memcpy(this->preferences_.devicetag, data + 1, size - 1);
        this->preferences_.devicetag[size - 1] = 0;
        this->preferences_object.save(&this->preferences_);
        data[0] = CMD_NODE_TAG_SET_REP;
        this->mesh_->commandReply(data, 1);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_CHANNEL_SET_REQ:
      if (size == 2) {
        uint8_t channel = data[1];
        if (channel < MAX_CHANNEL) {
          this->preferences_.channel = channel;
          this->preferences_object.save(&this->preferences_);
          data[0] = CMD_CHANNEL_SET_REP;
          this->mesh_->commandReply(data, 1);
          return HANDLE_UART_OK;
        }
      }
      break;
    case CMD_NODE_CONFIG_REQ:
      if (size == 1) {
        uint8_t reply[sizeof(MeshmeshSettings) + 1];
        reply[0] = CMD_NODE_CONFIG_REP;
        memcpy(reply + 1, &this->preferences_, sizeof(MeshmeshSettings));
        this->mesh_->commandReply(reply, sizeof(MeshmeshSettings) + 1);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_LOG_DEST_REQ:
      if (size == 1) {
        uint8_t reply[5] = {0};
        reply[0] = CMD_LOG_DEST_REP;
        espmeshmesh::uint32toBuffer(reply + 1, this->preferences_.log_destination);
        this->mesh_->commandReply(reply, 5);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_LOG_DEST_SET_REQ:
      if (size == 5) {
        this->preferences_.log_destination = espmeshmesh::uint32FromBuffer(data + 1);
        this->preferences_object.save(&this->preferences_);
        data[0] = CMD_LOG_DEST_SET_REP;
        this->mesh_->commandReply(data, 1);
        return HANDLE_UART_OK;
      }
      break;
      case CMD_GROUPS_REQ:
      if (size == 1) {
        uint8_t reply[5] = {0};
        reply[0] = CMD_GROUPS_REP;
        espmeshmesh::uint32toBuffer(reply + 1, this->preferences_.groups);
        this->mesh_->commandReply(reply, 5);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_GROUPS_SET_REQ:
      if (size == 5) {
        this->preferences_.groups = espmeshmesh::uint32FromBuffer(data + 1);
        this->preferences_object.save(&this->preferences_);
        data[0] = CMD_GROUPS_SET_REP;
        this->mesh_->commandReply(data, 1);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_FIRMWARE_REQ:
      if (size == 1) {
        size_t reply_size = 1 + strlen(ESPHOME_VERSION) + 1 + App.get_compilation_time().length() + 1;
        uint8_t *reply = new uint8_t[reply_size];
        reply[0] = CMD_FIRMWARE_REP;
        strcpy((char *) reply + 1, ESPHOME_VERSION);
        reply[1 + strlen(ESPHOME_VERSION)] = ' ';
        strcpy((char *) reply + 1 + strlen(ESPHOME_VERSION) + 1, App.get_compilation_time().c_str());
        reply[size - 1] = 0;
        this->mesh_->commandReply((const uint8_t *) reply, reply_size);
        delete[] reply;
        return HANDLE_UART_OK;
      }
      break;
    case CMD_REBOOT_REQ:
      if (size == 1) {
        data[0] = CMD_REBOOT_REP;
        this->reboot_requested_ = true;
        this->reboot_requested_time_ = millis();
        this->mesh_->commandReply(data, 1);
        return HANDLE_UART_OK;
      }
      break;

    default:
      return FRAME_NOT_HANDLED;
  }
  return FRAME_NOT_HANDLED;
}

void logPrintfCb(int level, const char *tag, int line, const char *format, va_list args) {
  esp_log_vprintf_(level, tag, line, format, args);
}

}  // namespace meshmesh
