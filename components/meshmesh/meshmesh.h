#pragma once
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif

#ifdef USE_ESP8266
extern "C" {
  void IRAM_ATTR __wrap_ppEnqueueRxq(void *a);
  void IRAM_ATTR __real_ppEnqueueRxq(void *);
}
#endif

namespace espmeshmesh {
  class EspMeshMesh;
  class MeshAddress;
}

namespace esphome {
namespace meshmesh {

enum UARTSelection {
  UART_SELECTION_UART0 = 0,
  UART_SELECTION_UART1,
  UART_SELECTION_UART2,
};

struct MeshmeshSettings {
  char devicetag[32];
  // Log destination
  uint32_t log_destination;
  uint8_t channel;
  uint8_t txPower;
  uint8_t flags;
  uint32_t groups;
} __attribute__((packed));

#ifdef USE_LOGGER
#define LOG_LISTENER ,logger::LogListener
#endif

class MeshmeshComponent : public Component LOG_LISTENER {
public:
  explicit MeshmeshComponent(int baud_rate, int tx_buffer, int rx_buffer);
  espmeshmesh::EspMeshMesh *getNetwork() { return mesh; }
  void setChannel(int channel) { mConfigChannel = channel; }
  void setAesPassword(const char *password);
  void setIsCoordinator() { mConfigIsCoordinator = true; }
  void setNodeType(int node_type) { mConfigNodeType = node_type; }
  void set_uart_selection(UARTSelection uart_selection) { /*FIXME: uart_ = uart_selection;*/}
  bool is_connected() { return mIsConnected; }
private:
  void defaultPreferences();
  void preSetupPreferences();
private:
  ESPPreferenceObject mPreferencesObject;
  MeshmeshSettings mPreferences;
private:
  uint8_t mConfigChannel;
  bool mConfigIsCoordinator{false};
  int mConfigNodeType{0};
public:
  void pre_setup();
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::BEFORE_CONNECTION; }
  void loop() override;
  void on_shutdown() override;
  bool teardown() override;
private:
  int8_t handleFrame(const uint8_t *data, uint16_t size,const espmeshmesh::MeshAddress &from, int16_t rssi);
#ifdef USE_LOGGER
  void on_log(uint8_t level, const char *tag, const char *message, size_t message_len) override;
#endif
  espmeshmesh::EspMeshMesh *mesh;
private:
  bool mLogToUart{false};
  bool mIsConnected{true};
  bool mRebootRequested{false};
  uint32_t mRebootRequestedTime{0};
};

extern MeshmeshComponent *global_meshmesh_component;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void logPrintfCb(int level, const char *tag, int line, const char *format, va_list args);

}  // namespace meshmesh
}  // namespace esphome

