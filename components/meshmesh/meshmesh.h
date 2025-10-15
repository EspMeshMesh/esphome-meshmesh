#pragma once
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/logger/logger.h"

#ifdef USE_ESP8266
extern "C" {
  void IRAM_ATTR __wrap_ppEnqueueRxq(void *a);
  void IRAM_ATTR __real_ppEnqueueRxq(void *);
}
#endif

namespace espmeshmesh {
  class EspMeshMesh;
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
#ifdef USE_BONDING_MODE
  uint32_t bonded_node;
#endif
} __attribute__((packed));

class MeshmeshComponent : public Component {
public:
  explicit MeshmeshComponent(int baud_rate, int tx_buffer, int rx_buffer);
  espmeshmesh::EspMeshMesh *getNetwork() { return mesh; }
  void setChannel(int channel) { mConfigChannel = channel; }
  void setAesPassword(const char *password);
  void set_uart_selection(UARTSelection uart_selection) { /*FIXME: uart_ = uart_selection;*/}
private:
  void defaultPreferences();
  void preSetupPreferences();
private:
  ESPPreferenceObject mPreferencesObject;
  MeshmeshSettings mPreferences;
private:
uint8_t mConfigChannel;
public:
  void pre_setup();
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::BEFORE_CONNECTION; }
  void loop() override;
private:
  int8_t handleFrame(const uint8_t *buf, uint16_t len, uint32_t from);
#ifdef USE_LOGGER
  void sendLog(int level, const char *tag, const char *payload);
#endif
  espmeshmesh::EspMeshMesh *mesh;
private:
  bool mLogToUart{false};
  bool mRebootRequested{false};
  uint32_t mRebootRequestedTime{0};
};

extern MeshmeshComponent *global_meshmesh_component;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void logPrintfCb(int level, const char *tag, int line, const char *format, va_list args);

}  // namespace meshmesh
}  // namespace esphome

