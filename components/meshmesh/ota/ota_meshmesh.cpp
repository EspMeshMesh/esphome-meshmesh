#include "ota_meshmesh.h"

#ifdef USE_OTA

#ifdef USE_OTA_PASSWORD
#include "esphome/components/sha256/sha256.h"
#endif

#include "esphome/components/ota/ota_backend.h"
#include "esphome/components/ota/ota_backend_esp8266.h"
#include "esphome/components/ota/ota_backend_arduino_libretiny.h"
#include "esphome/components/ota/ota_backend_arduino_rp2040.h"
#include "esphome/components/ota/ota_backend_esp_idf.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"

#include <espmeshmesh.h>
#include <meshsocket.h>

#include <cstring>
#include <functional>

namespace esphome {
namespace meshmesh {

static const char *const TAG = "meshmesh_ota";
static constexpr uint16_t MESHMESH_OTA_PORT = 0xA5;
static constexpr uint16_t OTA_BLOCK_SIZE = 8192;
static constexpr size_t OTA_BUFFER_SIZE = 1024;
static constexpr uint32_t OTA_TIMEOUT_HANDSHAKE_MS = 20000;
static constexpr uint32_t OTA_TIMEOUT_DATA_MS = 90000;

static const uint8_t FEATURE_SUPPORTS_COMPRESSION = 0x01;
static const uint8_t FEATURE_SUPPORTS_SHA256_AUTH = 0x02;

void MeshmeshOtaComponent::setup() {
  this->m_socket_ = new espmeshmesh::MeshSocket(MESHMESH_OTA_PORT);
  this->m_socket_->recvDatagramCb(
      std::bind(&MeshmeshOtaComponent::handle_frame_, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));
  int8_t err = this->m_socket_->open();
  if (err != espmeshmesh::MeshSocket::errSuccess) {
    ESP_LOGE(TAG, "Error opening socket: %s", espmeshmesh::MeshSocket::error2string(
                                                    static_cast<espmeshmesh::MeshSocket::ErrorCodes>(err)));
    this->mark_failed();
  }
}

void MeshmeshOtaComponent::on_shutdown() {
  if (this->m_socket_ != nullptr) {
    this->m_socket_->close();
    delete this->m_socket_;
    this->m_socket_ = nullptr;
  }
}

float MeshmeshOtaComponent::get_setup_priority() const { return setup_priority::AFTER_WIFI; }

void MeshmeshOtaComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MeshMesh OTA (datagram port 0x%02X, protocol version %d)", MESHMESH_OTA_PORT, USE_OTA_VERSION);
#ifdef USE_OTA_PASSWORD
  if (!this->password_.empty()) {
    ESP_LOGCONFIG(TAG, "  Password configured");
  }
#endif
}

void MeshmeshOtaComponent::loop() {
  if (this->m_socket_ == nullptr) {
    return;
  }
  if (!this->has_peer_ && this->rx_available_() == 0) {
    return;
  }

  if (this->ota_state_ == OTAState::IDLE) {
    if (this->rx_available_() > 0) {
      this->has_peer_ = true;
      this->session_start_ms_ = millis();
      this->transition_ota_state_(OTAState::MAGIC_READ);
      ESP_LOGD(TAG, "Starting handshake from %06X", this->peer_.address);
    } else {
      return;
    }
  }

  if (this->ota_state_ < OTAState::DATA_AUTH_OK) {
    if (this->check_timeout_(OTA_TIMEOUT_HANDSHAKE_MS)) {
      ESP_LOGW(TAG, "Handshake timeout");
      this->cleanup_session_();
      return;
    }
    this->handle_handshake_();
  } else {
    if (this->check_timeout_(OTA_TIMEOUT_DATA_MS)) {
      ESP_LOGW(TAG, "Data transfer timeout");
      this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_UNKNOWN);
      return;
    }
    this->handle_data_step_();
  }
}

void MeshmeshOtaComponent::handle_frame_(const uint8_t *buf, uint16_t len, const espmeshmesh::MeshAddress &from,
                                          int16_t rssi) {
  (void) rssi;
  if (len == 0) {
    return;
  }

  if (this->has_peer_ && from.address != this->peer_.address) {
    ESP_LOGV(TAG, "Ignoring datagram from %06X (session peer %06X)", from.address, this->peer_.address);
    return;
  }

  if (!this->has_peer_) {
    this->peer_ = from;
    this->peer_.port = MESHMESH_OTA_PORT;
  }

  if (this->rx_available_() + len > RX_RING_MAX) {
    ESP_LOGE(TAG, "RX ring overflow, aborting session");
    this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_UNKNOWN);
    return;
  }

  this->rx_append_(buf, len);
}

void MeshmeshOtaComponent::rx_append_(const uint8_t *data, uint16_t len) {
  this->rx_buf_.insert(this->rx_buf_.end(), data, data + len);
}

size_t MeshmeshOtaComponent::rx_available_() const {
  if (this->rx_read_pos_ >= this->rx_buf_.size()) {
    return 0;
  }
  return this->rx_buf_.size() - this->rx_read_pos_;
}

ssize_t MeshmeshOtaComponent::rx_read_(uint8_t *dst, size_t len) {
  const size_t avail = this->rx_available_();
  if (avail == 0) {
    return 0;
  }
  const size_t to_copy = avail < len ? avail : len;
  memcpy(dst, this->rx_buf_.data() + this->rx_read_pos_, to_copy);
  this->rx_read_pos_ += to_copy;
  if (this->rx_read_pos_ >= 4096 && this->rx_read_pos_ == this->rx_buf_.size()) {
    this->rx_buf_.clear();
    this->rx_read_pos_ = 0;
  } else if (this->rx_read_pos_ >= 4096) {
    this->rx_buf_.erase(this->rx_buf_.begin(), this->rx_buf_.begin() + this->rx_read_pos_);
    this->rx_read_pos_ = 0;
  }
  return static_cast<ssize_t>(to_copy);
}

bool MeshmeshOtaComponent::send_to_peer_(const uint8_t *data, uint16_t len) {
  if (this->m_socket_ == nullptr || !this->has_peer_) {
    return false;
  }
  espmeshmesh::MeshAddress target = this->peer_;
  target.port = MESHMESH_OTA_PORT;
  int16_t sent = this->m_socket_->sendDatagram(data, len, target, nullptr);
  if (sent < 0) {
    ESP_LOGW(TAG, "sendDatagram failed: %s",
             espmeshmesh::MeshSocket::error2string(static_cast<espmeshmesh::MeshSocket::ErrorCodes>(sent)));
    return false;
  }
  return true;
}

bool MeshmeshOtaComponent::try_read_(size_t to_read) {
  const size_t need = to_read - this->handshake_buf_pos_;
  const ssize_t got = this->rx_read_(this->handshake_buf_ + this->handshake_buf_pos_, need);
  if (got <= 0) {
    return false;
  }
  this->handshake_buf_pos_ += static_cast<size_t>(got);
  return this->handshake_buf_pos_ >= to_read;
}

bool MeshmeshOtaComponent::try_write_(size_t to_write) {
  const size_t remaining = to_write - this->handshake_buf_pos_;
  if (!this->send_to_peer_(this->handshake_buf_ + this->handshake_buf_pos_, static_cast<uint16_t>(remaining))) {
    return false;
  }
  this->handshake_buf_pos_ = to_write;
  return true;
}

bool MeshmeshOtaComponent::readall_(uint8_t *buf, size_t len) {
  size_t at = 0;
  while (at < len) {
    const ssize_t got = this->rx_read_(buf + at, len - at);
    if (got > 0) {
      at += static_cast<size_t>(got);
      continue;
    }
    this->yield_and_feed_watchdog_();
    if (this->check_timeout_(OTA_TIMEOUT_DATA_MS)) {
      return false;
    }
  }
  return true;
}

bool MeshmeshOtaComponent::writeall_(const uint8_t *buf, size_t len) {
  return this->send_to_peer_(buf, static_cast<uint16_t>(len));
}

bool MeshmeshOtaComponent::write_byte_(uint8_t byte) { return this->writeall_(&byte, 1); }

void MeshmeshOtaComponent::transition_ota_state_(OTAState next_state) {
  this->ota_state_ = next_state;
  this->handshake_buf_pos_ = 0;
}

bool MeshmeshOtaComponent::check_timeout_(uint32_t timeout_ms) {
  const uint32_t start =
      (this->ota_state_ < OTAState::DATA_AUTH_OK) ? this->session_start_ms_ : this->data_phase_start_ms_;
  return (millis() - start) > timeout_ms;
}

void MeshmeshOtaComponent::yield_and_feed_watchdog_() {
  if (global_meshmesh_component != nullptr) {
    global_meshmesh_component->loop();
  }
  App.feed_wdt();
#ifdef USE_ESP8266
  delay(5);
#else
  delay(1);
#endif
}

void MeshmeshOtaComponent::log_start_(const char *phase) {
  ESP_LOGD(TAG, "Starting %s from %06X", phase, this->peer_.address);
}

void MeshmeshOtaComponent::log_read_error_(const char *what) { ESP_LOGW(TAG, "Read %s failed", what); }

void MeshmeshOtaComponent::send_error_and_cleanup_(ota::OTAResponseTypes error) {
  const uint8_t err_byte = static_cast<uint8_t>(error);
  this->send_to_peer_(&err_byte, 1);
  if (this->backend_ != nullptr && this->update_started_) {
    this->backend_->abort();
  }
  this->cleanup_session_();
}

void MeshmeshOtaComponent::cleanup_session_() {
  this->has_peer_ = false;
  this->peer_ = espmeshmesh::MeshAddress{};
  this->rx_buf_.clear();
  this->rx_read_pos_ = 0;
  this->handshake_buf_pos_ = 0;
  this->ota_state_ = OTAState::IDLE;
  this->ota_features_ = 0;
  this->backend_ = nullptr;
  this->session_start_ms_ = 0;
  this->data_phase_start_ms_ = 0;
  this->ota_size_ = 0;
  this->firmware_total_ = 0;
  this->update_started_ = false;
#if USE_OTA_VERSION == 2
  this->size_acknowledged_ = 0;
#endif
#ifdef USE_OTA_PASSWORD
  this->cleanup_auth_();
#endif
}

void MeshmeshOtaComponent::handle_handshake_() {
  switch (this->ota_state_) {
    case OTAState::MAGIC_READ: {
      if (!this->try_read_(5)) {
        return;
      }
      static const uint8_t MAGIC_BYTES[5] = {0x6C, 0x26, 0xF7, 0x5C, 0x45};
      if (memcmp(this->handshake_buf_, MAGIC_BYTES, 5) != 0) {
        ESP_LOGW(TAG, "Magic bytes mismatch");
        this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_MAGIC);
        return;
      }
      this->transition_ota_state_(OTAState::MAGIC_ACK);
      this->handshake_buf_[0] = ota::OTA_RESPONSE_OK;
      this->handshake_buf_[1] = USE_OTA_VERSION;
      [[fallthrough]];
    }
    case OTAState::MAGIC_ACK: {
      if (!this->try_write_(2)) {
        return;
      }
      this->backend_ = ota::make_ota_backend();
      this->transition_ota_state_(OTAState::FEATURE_READ);
      [[fallthrough]];
    }
    case OTAState::FEATURE_READ: {
      if (!this->try_read_(1)) {
        return;
      }
      this->ota_features_ = this->handshake_buf_[0];
      this->transition_ota_state_(OTAState::FEATURE_ACK);
      this->handshake_buf_[0] =
          ((this->ota_features_ & FEATURE_SUPPORTS_COMPRESSION) != 0 && this->backend_->supports_compression())
              ? ota::OTA_RESPONSE_SUPPORTS_COMPRESSION
              : ota::OTA_RESPONSE_HEADER_OK;
      [[fallthrough]];
    }
    case OTAState::FEATURE_ACK: {
      if (!this->try_write_(1)) {
        return;
      }
#ifdef USE_OTA_PASSWORD
      if (!this->password_.empty()) {
        this->transition_ota_state_(OTAState::AUTH_SEND);
        return;
      }
#endif
      this->data_phase_start_ms_ = millis();
      this->transition_ota_state_(OTAState::DATA_AUTH_OK);
      return;
    }

#ifdef USE_OTA_PASSWORD
    case OTAState::AUTH_SEND: {
      if (!this->handle_auth_send_()) {
        return;
      }
      this->transition_ota_state_(OTAState::AUTH_READ);
      [[fallthrough]];
    }
    case OTAState::AUTH_READ: {
      if (!this->handle_auth_read_()) {
        return;
      }
      this->data_phase_start_ms_ = millis();
      this->transition_ota_state_(OTAState::DATA_AUTH_OK);
      return;
    }
#endif

    default:
      break;
  }
}

void MeshmeshOtaComponent::handle_data_step_() {
  ota::OTAResponseTypes error_code = ota::OTA_RESPONSE_ERROR_UNKNOWN;

  switch (this->ota_state_) {
    case OTAState::DATA_AUTH_OK: {
      if (!this->write_byte_(ota::OTA_RESPONSE_AUTH_OK)) {
        return;
      }
      this->transition_ota_state_(OTAState::DATA_SIZE);
      [[fallthrough]];
    }
    case OTAState::DATA_SIZE: {
      if (!this->readall_(this->data_buf_, 4)) {
        this->log_read_error_("size");
        this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_UNKNOWN);
        return;
      }
      this->ota_size_ = (static_cast<size_t>(this->data_buf_[0]) << 24) |
                        (static_cast<size_t>(this->data_buf_[1]) << 16) |
                        (static_cast<size_t>(this->data_buf_[2]) << 8) | this->data_buf_[3];
      ESP_LOGV(TAG, "OTA size %u bytes", this->ota_size_);
      this->log_start_("update");
      this->transition_ota_state_(OTAState::DATA_BEGIN);
      [[fallthrough]];
    }
    case OTAState::DATA_BEGIN: {
      error_code = this->backend_->begin(this->ota_size_);
      if (error_code != ota::OTA_RESPONSE_OK) {
        this->send_error_and_cleanup_(error_code);
        return;
      }
      this->update_started_ = true;
      this->firmware_total_ = 0;
      this->last_progress_ms_ = millis();
#if USE_OTA_VERSION == 2
      this->size_acknowledged_ = 0;
#endif
      this->transition_ota_state_(OTAState::DATA_PREPARE_OK);
      [[fallthrough]];
    }
    case OTAState::DATA_PREPARE_OK: {
      if (!this->write_byte_(ota::OTA_RESPONSE_UPDATE_PREPARE_OK)) {
        return;
      }
      this->transition_ota_state_(OTAState::DATA_MD5);
      [[fallthrough]];
    }
    case OTAState::DATA_MD5: {
      if (!this->readall_(this->data_buf_, 32)) {
        this->log_read_error_("MD5 checksum");
        this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_UNKNOWN);
        return;
      }
      char md5_str[33];
      memcpy(md5_str, this->data_buf_, 32);
      md5_str[32] = '\0';
      this->backend_->set_update_md5(md5_str);
      this->transition_ota_state_(OTAState::DATA_MD5_OK);
      [[fallthrough]];
    }
    case OTAState::DATA_MD5_OK: {
      if (!this->write_byte_(ota::OTA_RESPONSE_BIN_MD5_OK)) {
        return;
      }
      this->transition_ota_state_(OTAState::DATA_FIRMWARE);
      return;
    }
    case OTAState::DATA_FIRMWARE: {
      if (this->firmware_total_ >= this->ota_size_) {
        this->transition_ota_state_(OTAState::DATA_RECEIVE_OK);
        return;
      }
      const size_t remaining = this->ota_size_ - this->firmware_total_;
      const size_t requested = remaining < OTA_BUFFER_SIZE ? remaining : OTA_BUFFER_SIZE;
      if (this->rx_available_() == 0) {
        this->yield_and_feed_watchdog_();
        return;
      }
      const ssize_t read = this->rx_read_(this->data_buf_, requested);
      if (read <= 0) {
        this->yield_and_feed_watchdog_();
        return;
      }

#ifdef USE_ESP8266
      if (global_meshmesh_component != nullptr) {
        global_meshmesh_component->getNetwork()->setLockdownMode(true);
      }
#endif
      error_code = this->backend_->write(this->data_buf_, static_cast<size_t>(read));
#ifdef USE_ESP8266
      if (global_meshmesh_component != nullptr) {
        global_meshmesh_component->getNetwork()->setLockdownMode(false);
      }
#endif

      if (error_code != ota::OTA_RESPONSE_OK) {
        ESP_LOGW(TAG, "Flash write err %d", error_code);
        this->send_error_and_cleanup_(error_code);
        return;
      }
      this->firmware_total_ += static_cast<size_t>(read);

#if USE_OTA_VERSION == 2
      while (this->size_acknowledged_ + OTA_BLOCK_SIZE <= this->firmware_total_ ||
             (this->firmware_total_ == this->ota_size_ && this->size_acknowledged_ < this->ota_size_)) {
        if (!this->write_byte_(ota::OTA_RESPONSE_CHUNK_OK)) {
          return;
        }
        this->size_acknowledged_ += OTA_BLOCK_SIZE;
      }
#endif

      const uint32_t now = millis();
      if (now - this->last_progress_ms_ > 1000) {
        this->last_progress_ms_ = now;
        ESP_LOGD(TAG, "Progress: %0.1f%%", (this->firmware_total_ * 100.0f) / this->ota_size_);
        this->yield_and_feed_watchdog_();
      }
      return;
    }
    case OTAState::DATA_RECEIVE_OK: {
      if (!this->write_byte_(ota::OTA_RESPONSE_RECEIVE_OK)) {
        return;
      }
      this->transition_ota_state_(OTAState::DATA_END);
      [[fallthrough]];
    }
    case OTAState::DATA_END: {
      error_code = this->backend_->end();
      if (error_code != ota::OTA_RESPONSE_OK) {
        ESP_LOGW(TAG, "End update err %d", error_code);
        this->send_error_and_cleanup_(error_code);
        return;
      }
      this->transition_ota_state_(OTAState::DATA_END_OK);
      [[fallthrough]];
    }
    case OTAState::DATA_END_OK: {
      if (!this->write_byte_(ota::OTA_RESPONSE_UPDATE_END_OK)) {
        return;
      }
      this->transition_ota_state_(OTAState::DATA_FINAL_ACK);
      return;
    }
    case OTAState::DATA_FINAL_ACK: {
      if (this->rx_available_() == 0) {
        this->yield_and_feed_watchdog_();
        return;
      }
      const ssize_t got = this->rx_read_(this->data_buf_, 1);
      if (got < 1) {
        return;
      }
      if (this->data_buf_[0] != ota::OTA_RESPONSE_OK) {
        ESP_LOGW(TAG, "Final ACK mismatch");
      }
      this->transition_ota_state_(OTAState::DATA_REBOOT);
      [[fallthrough]];
    }
    case OTAState::DATA_REBOOT: {
      ESP_LOGI(TAG, "Update complete");
      this->cleanup_session_();
      delay(100);
      App.safe_reboot();
      return;
    }
    default:
      break;
  }
}

#ifdef USE_OTA_PASSWORD
void MeshmeshOtaComponent::log_auth_warning_(const char *msg) { ESP_LOGW(TAG, "Auth: %s", msg); }

bool MeshmeshOtaComponent::select_auth_type_() {
  if ((this->ota_features_ & FEATURE_SUPPORTS_SHA256_AUTH) == 0) {
    this->log_auth_warning_("SHA256 required");
    this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_AUTH_INVALID);
    return false;
  }
  this->auth_type_ = ota::OTA_RESPONSE_REQUEST_SHA256_AUTH;
  return true;
}

bool MeshmeshOtaComponent::handle_auth_send_() {
  if (!this->auth_buf_) {
    if (!this->select_auth_type_()) {
      return false;
    }

    sha256::SHA256 hasher;
    const size_t hex_size = hasher.get_size() * 2;
    const size_t nonce_len = hasher.get_size() / 4;
    const size_t auth_buf_size = 1 + 3 * hex_size;
    this->auth_buf_ = std::make_unique<uint8_t[]>(auth_buf_size);
    this->auth_buf_pos_ = 0;

    char *buf = reinterpret_cast<char *>(this->auth_buf_.get() + 1);
    if (!random_bytes(reinterpret_cast<uint8_t *>(buf), nonce_len)) {
      this->log_auth_warning_("Random failed");
      this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_UNKNOWN);
      return false;
    }

    hasher.init();
    hasher.add(buf, nonce_len);
    hasher.calculate();
    this->auth_buf_[0] = this->auth_type_;
    hasher.get_hex(buf);
  }

  constexpr size_t hex_size = SHA256_HEX_SIZE;
  const size_t to_write = 1 + hex_size;
  const size_t remaining = to_write - this->auth_buf_pos_;
  if (!this->send_to_peer_(this->auth_buf_.get() + this->auth_buf_pos_, static_cast<uint16_t>(remaining))) {
    return false;
  }
  this->auth_buf_pos_ = to_write;
  this->auth_buf_pos_ = 0;
  return true;
}

bool MeshmeshOtaComponent::handle_auth_read_() {
  constexpr size_t hex_size = SHA256_HEX_SIZE;
  const size_t to_read = hex_size * 2;
  const size_t cnonce_offset = 1 + hex_size;

  const size_t need = to_read - this->auth_buf_pos_;
  const ssize_t got =
      this->rx_read_(this->auth_buf_.get() + cnonce_offset + this->auth_buf_pos_, need);
  if (got <= 0) {
    return false;
  }
  this->auth_buf_pos_ += static_cast<size_t>(got);
  if (this->auth_buf_pos_ < to_read) {
    return false;
  }

  const char *nonce = reinterpret_cast<char *>(this->auth_buf_.get() + 1);
  const char *cnonce = nonce + hex_size;
  const char *response = cnonce + hex_size;

  sha256::SHA256 hasher;
  hasher.init();
  hasher.add(this->password_.c_str(), this->password_.length());
  hasher.add(nonce, hex_size * 2);
  hasher.calculate();

  if (!hasher.equals_hex(response)) {
    this->log_auth_warning_("Password mismatch");
    this->send_error_and_cleanup_(ota::OTA_RESPONSE_ERROR_AUTH_INVALID);
    return false;
  }

  this->cleanup_auth_();
  return true;
}

void MeshmeshOtaComponent::cleanup_auth_() {
  this->auth_buf_ = nullptr;
  this->auth_buf_pos_ = 0;
  this->auth_type_ = 0;
}
#endif  // USE_OTA_PASSWORD

}  // namespace meshmesh
}  // namespace esphome

#endif  // USE_OTA
