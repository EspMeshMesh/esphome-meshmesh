#pragma once

#include "esphome/core/defines.h"
#ifdef USE_OTA

#include "esphome/components/ota/ota_backend_factory.h"
#include "esphome/components/meshmesh/meshmesh.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <espmeshmesh.h>
#include <meshsocket.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace esphome {
namespace meshmesh {

class MeshmeshOtaComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void on_shutdown() override;

#ifdef USE_OTA_PASSWORD
  void set_auth_password(const std::string &password) { this->password_ = password; }
#endif

 protected:
  enum class OTAState : uint8_t {
    IDLE,
    MAGIC_READ,
    MAGIC_ACK,
    FEATURE_READ,
    FEATURE_ACK,
#ifdef USE_OTA_PASSWORD
    AUTH_SEND,
    AUTH_READ,
#endif
    DATA_AUTH_OK,
    DATA_SIZE,
    DATA_BEGIN,
    DATA_PREPARE_OK,
    DATA_MD5,
    DATA_MD5_OK,
    DATA_FIRMWARE,
    DATA_RECEIVE_OK,
    DATA_END,
    DATA_END_OK,
    DATA_FINAL_ACK,
    DATA_REBOOT,
  };

  void handle_frame_(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from, int16_t rssi);
  void handle_handshake_();
  void handle_data_step_();

#ifdef USE_OTA_PASSWORD
  bool handle_auth_send_();
  bool handle_auth_read_();
  bool select_auth_type_();
  void cleanup_auth_();
  void log_auth_warning_(const char *msg);
#endif

  bool try_read_(size_t to_read);
  bool try_write_(size_t to_write);
  bool readall_(uint8_t *buf, size_t len);
  bool writeall_(const uint8_t *buf, size_t len);
  bool write_byte_(uint8_t byte);

  void rx_append_(const uint8_t *data, uint16_t len);
  size_t rx_available_() const;
  ssize_t rx_read_(uint8_t *dst, size_t len);

  bool send_to_peer_(const uint8_t *data, uint16_t len);
  void transition_ota_state_(OTAState next_state);
  void send_error_and_cleanup_(ota::OTAResponseTypes error);
  void cleanup_session_();
  void yield_and_feed_watchdog_();
  void log_start_(const char *phase);
  void log_read_error_(const char *what);
  bool check_timeout_(uint32_t timeout_ms);

  espmeshmesh::MeshSocket *m_socket_{nullptr};
  espmeshmesh::MeshAddress peer_{};
  bool has_peer_{false};

  static constexpr size_t RX_RING_MAX = 8192;
  static constexpr size_t HANDSHAKE_BUF_SIZE = 128;
  std::vector<uint8_t> rx_buf_;
  size_t rx_read_pos_{0};

  uint8_t handshake_buf_[HANDSHAKE_BUF_SIZE];
  size_t handshake_buf_pos_{0};
  OTAState ota_state_{OTAState::IDLE};
  uint8_t ota_features_{0};
  ota::OTABackendPtr backend_;

  uint32_t session_start_ms_{0};
  uint32_t data_phase_start_ms_{0};
  uint32_t last_progress_ms_{0};

  size_t ota_size_{0};
  size_t firmware_total_{0};
#if USE_OTA_VERSION == 2
  size_t size_acknowledged_{0};
#endif
  bool update_started_{false};

  uint8_t data_buf_[1024];

#ifdef USE_OTA_PASSWORD
  static constexpr size_t SHA256_HEX_SIZE = 64;
  std::string password_;
  std::unique_ptr<uint8_t[]> auth_buf_;
  size_t auth_buf_pos_{0};
  uint8_t auth_type_{0};
#endif
};

}  // namespace meshmesh
}  // namespace esphome

#endif  // USE_OTA
