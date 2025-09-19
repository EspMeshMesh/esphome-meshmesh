#pragma once
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#ifdef USE_ESP8266
extern "C"
{
  void IRAM_ATTR __wrap_ppEnqueueRxq(void *a);
  void IRAM_ATTR __real_ppEnqueueRxq(void *);
}
#endif

namespace espmeshmesh
{
  class EspMeshMesh;
}

namespace esphome::meshmesh
{

  enum UARTSelection
  {
    UART_SELECTION_UART0 = 0,
    UART_SELECTION_UART1,
    UART_SELECTION_UART2,
  };

  struct MeshmeshSettings
  {
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

  class MeshmeshComponent : public Component
  {
  public:
    explicit MeshmeshComponent(int baud_rate, int tx_buffer, int rx_buffer);

    void pre_setup();

    void setup() override;
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::BEFORE_CONNECTION; }
    void loop() override;

    void set_channel(int channel) { this->config_channel_ = channel; }
    void set_password(const char *password);
    void set_uart_selection(UARTSelection uart_selection) { /*FIXME: uart_ = uart_selection;*/ }

    espmeshmesh::EspMeshMesh *getNetwork() { return this->mesh; }

    private:
    void default_preferences_();
    void pre_setup_preferences_();

    int8_t handle_frame_(uint8_t *buf, uint16_t size, uint32_t from);

    bool reboot_requested_{false};
    uint32_t reboot_requested_time_{0};

    espmeshmesh::EspMeshMesh *mesh_;

    ESPPreferenceObject preferences_object_;
    MeshmeshSettings preferences_;

    uint8_t config_channel_;
  };

  void logPrintfCb(int level, const char *tag, int line, const char *format, va_list args);

} // namespace meshmesh
