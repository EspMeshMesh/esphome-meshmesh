#include "meshmesh.h"
#include "commands.h"
#include "packetbuf.h"
#include "discovery.h"
#include <espmeshmesh.h>

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include "esphome/core/application.h"
#include "esphome/core/version.h"
#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif

extern "C" {
#include <pb_encode.h>
#include "nodeinfo.pb.h"
}

#define MAX_CHANNEL 13

static const char *TAG = "meshmesh";

#ifdef USE_ESP8266
void IRAM_ATTR HOT __wrap_ppEnqueueRxq(void *a) {
	// 4 is the only spot that contained the packets. Discovered by trial and error printing the data
    if(espmeshmesh::PacketBuf::singleton) espmeshmesh::PacketBuf::singleton->rawRecv((espmeshmesh::RxPacket *)(((void **)a)[4]));
	__real_ppEnqueueRxq(a);
}
#endif

namespace esphome {
namespace meshmesh {

  MeshmeshComponent *global_meshmesh_component =  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  nullptr;                                        // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)


MeshmeshComponent::MeshmeshComponent(int baud_rate, int tx_buffer, int rx_buffer) {
  global_meshmesh_component = this;
  mLogToUart = baud_rate > 0;
  mesh = new espmeshmesh::EspMeshMesh(baud_rate, tx_buffer, rx_buffer);
  mesh->setLogCb(logPrintfCb);
}

void MeshmeshComponent::setAesPassword(const char *password) {
  this->mesh->setAesPassword(password);
}

void MeshmeshComponent::defaultPreferences() {
  // Default preferences
  memset(mPreferences.devicetag, 0, 32);
  mPreferences.channel = UINT8_MAX;
  mPreferences.txPower = UINT8_MAX;
  mPreferences.flags = 0;
  mPreferences.log_destination = 0;
  mPreferences.groups = 0;
}

void MeshmeshComponent::preSetupPreferences() {
  defaultPreferences();
  mPreferencesObject = global_preferences->make_preference<MeshmeshSettings>(fnv1_hash("MeshmeshComponent"), true);
  if (!mPreferencesObject.load(&mPreferences)) {
    ESP_LOGE(TAG, "Can't read prederences from flash");
  }
}


void MeshmeshComponent::pre_setup() {
  preSetupPreferences();
  mesh->pre_setup();
}

void MeshmeshComponent::setup() {
  espmeshmesh::EspMeshMeshSetupConfig config = {
    .hostname = App.get_name(),
    .channel = mPreferences.channel == UINT8_MAX ? mConfigChannel : mPreferences.channel,
    .txPower = mPreferences.txPower,
    .isCoordinator = mConfigIsCoordinator,
    .fwVersion = ESPHOME_VERSION,
    .compileTime = App.get_compilation_time()
  };

  mesh->setup(&config);
  mesh->addHandleFrameCb(std::bind(&MeshmeshComponent::handleFrame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void MeshmeshComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Meshmesh");
  ESP_LOGCONFIG(TAG, "Is Coordinator: %d", mConfigIsCoordinator);
#ifdef USE_ESP32
  ESP_LOGCONFIG(TAG, "Sys cip ID: %08lX", espmeshmesh::Discovery::chipId());
#else
  ESP_LOGCONFIG(TAG, "Sys cip ID: %08X", system_get_chip_id());
  ESP_LOGCONFIG(TAG, "Curr. Channel: %d Saved Channel: %d", wifi_get_channel(), mPreferences.channel);
#endif
}

void MeshmeshComponent::loop() {
  mesh->loop();

  if (mRebootRequested) {
    if (millis() - mRebootRequestedTime > 250) {
      App.reboot();
    }
  }
}

void MeshmeshComponent::on_shutdown() {
  mesh->shutdown();
}

bool MeshmeshComponent::teardown() {
  bool teardown_complete = mesh->teardown();
  if(teardown_complete) mIsConnected = false;
  return teardown_complete;
}

int8_t MeshmeshComponent::handleFrame(const uint8_t *data, uint16_t size,const espmeshmesh::MeshAddress &from, int16_t rssi) {
  //ESP_LOGD(TAG, "handleFrame: %d, len: %d, from: %d", buf[0], len, from);
  switch (data[0]) {
    case CMD_NODE_TAG_REQ:
      if (size == 1) {
        uint8_t rep[33] = {0};
        rep[0] = CMD_NODE_TAG_REP;
        memcpy(rep + 1, mPreferences.devicetag, 32);
        mesh->commandReply(rep, 33);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_NODE_TAG_SET_REQ:
      if (size > 1) {
        memcpy(mPreferences.devicetag, data + 1, size - 1);
        mPreferences.devicetag[size - 1] = 0;
        mPreferencesObject.save(&mPreferences);
        uint8_t rep[1] = {CMD_NODE_TAG_SET_REP};
        mesh->commandReply(rep, 1);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_CHANNEL_SET_REQ:
      if (size == 2) {
        uint8_t channel = data[1];
        if (channel < MAX_CHANNEL) {
          mPreferences.channel = channel;
          mPreferencesObject.save(&mPreferences);
          uint8_t rep[1] = {CMD_CHANNEL_SET_REP};
          mesh->commandReply(rep, 1);
          return HANDLE_UART_OK;
        }
      }
      break;
    case CMD_NODE_CONFIG_REQ:
      if (size == 1) {
        uint8_t rep[sizeof(MeshmeshSettings) + 1];
        rep[0] = CMD_NODE_CONFIG_REP;
        memcpy(rep + 1, &mPreferences, sizeof(MeshmeshSettings));
        mesh->commandReply(rep, sizeof(MeshmeshSettings) + 1);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_NODE_INFO_REQ:
      if (size == 1) {
        std::string platform = "ESP";
        std::string mac_address = "00:00:00:00:00:00";

        pb_meshmesh_NodeInfo nodeinfo = pb_meshmesh_NodeInfo_init_default;
        strncpy(nodeinfo.friendly_name, App.get_friendly_name().c_str(), 48);
        strncpy(nodeinfo.firmware_version, ESPHOME_VERSION, 16);
        strncpy(nodeinfo.mac_address, mac_address.c_str(), 24);
        strncpy(nodeinfo.platform, platform.c_str(), 16);
        strncpy(nodeinfo.board, ESPHOME_BOARD, 32);
        strncpy(nodeinfo.compile_time, App.get_compilation_time().c_str(), 16);
        strncpy(nodeinfo.lib_version, mesh->libVersion().c_str(), 16);

        // Calculate the size of the message
        uint8_t cmdid = pb_meshmesh_NodeInfo_msgid;
        pb_ostream_t sizestream = {0};
        pb_write(&sizestream, &cmdid, 1);
        pb_encode(&sizestream, pb_meshmesh_NodeInfo_fields, &nodeinfo);
        
        // Encode the message
        uint8_t *buffer = new uint8_t[sizestream.bytes_written];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizestream.bytes_written);
        pb_write(&stream, &cmdid, 1);
        pb_encode(&stream, pb_meshmesh_NodeInfo_fields, &nodeinfo);
        
        // Encrypt and send the packet
        mesh->commandReply(buffer, sizestream.bytes_written);
        delete[] buffer;

        return HANDLE_UART_OK;
      }
      break;
    case CMD_LOG_DEST_REQ:
      if (size == 1) {
        uint8_t rep[5] = {0};
        rep[0] = CMD_LOG_DEST_REP;
        espmeshmesh::uint32toBuffer(rep + 1, mPreferences.log_destination);
        mesh->commandReply(rep, 5);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_LOG_DEST_SET_REQ:
      if (size == 5) {
        mPreferences.log_destination = espmeshmesh::uint32FromBuffer(data + 1);
        mPreferencesObject.save(&mPreferences);
        uint8_t rep[1] = {CMD_LOG_DEST_SET_REP};
        mesh->commandReply(rep, 1);
        return HANDLE_UART_OK;
      }
      break;
      case CMD_GROUPS_REQ:
      if (size == 1) {
        uint8_t rep[5] = {0};
        rep[0] = CMD_GROUPS_REP;
        espmeshmesh::uint32toBuffer(rep + 1, mPreferences.groups);
        mesh->commandReply(rep, 5);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_GROUPS_SET_REQ:
      if (size == 5) {
        mPreferences.groups = espmeshmesh::uint32FromBuffer(data + 1);
        mPreferencesObject.save(&mPreferences);
        uint8_t rep[1] = {CMD_GROUPS_SET_REP};
        mesh->commandReply(rep, 1);
        return HANDLE_UART_OK;
      }
      break;
    case CMD_FIRMWARE_REQ:
      if (size == 1) {
        size_t size = 1 + strlen(ESPHOME_VERSION) + 1;
        uint8_t *rep = new uint8_t[size];
        rep[0] = CMD_FIRMWARE_REP;
        strcpy((char *) rep + 1, ESPHOME_VERSION);
        mesh->commandReply((const uint8_t *) rep, size);
        delete[] rep;
        return HANDLE_UART_OK;
      }
      break;
    case CMD_REBOOT_REQ:
      if (size == 1) {
        mRebootRequested = true;
        mRebootRequestedTime = millis();
        uint8_t rep[1] = {CMD_REBOOT_REP};
        mesh->commandReply(rep, 1);
        return HANDLE_UART_OK;
      }
      break;

    default:
      return FRAME_NOT_HANDLED;
  }
  return FRAME_NOT_HANDLED;
}

void MeshmeshComponent::on_log(uint8_t level, const char *tag, const char *message, size_t message_len) {
  if (!mLogToUart && mPreferences.log_destination == 0)
    return;

  uint16_t buffersize = 7 + message_len;

  auto buffer = new uint8_t[buffersize];
  auto buffer_ptr = buffer;

  *buffer_ptr = CMD_LOGEVENT_REP;
  buffer_ptr++;
  espmeshmesh::uint16toBuffer(buffer_ptr, (uint16_t) level);
  buffer_ptr += 2;
  espmeshmesh::uint32toBuffer(buffer_ptr, 0);
  buffer_ptr += 4;

  if (message_len > 0)
    memcpy(buffer_ptr, message, message_len);
  if (mLogToUart)
    mesh->uartSendData(buffer, buffersize);
  if (mPreferences.log_destination == 1) {
  }

  delete[] buffer;
}
 
void logPrintfCb(int level, const char *tag, int line, const char *format, va_list args) {
  esp_log_vprintf_(level, tag, line, format, args);
}

}  // namespace meshmesh
}  // namespace esphome
