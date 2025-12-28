#include <queue>

#include "socket.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"

#ifdef USE_SOCKET_IMPL_MESHMESH_8266

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "esphome/components/meshmesh/meshmesh.h"

#include <espmeshmesh.h>
#include <connectedpath.h>

#define MAX_RX_QUEUE_SIZE 2048

namespace esphome {
namespace socket {

static const char *const TAG = "socket.meshmesh";

class MeshmeshRawImpl : public Socket {
 public:
  MeshmeshRawImpl(uint32_t from, uint16_t handle, bool server) : mFrom(from), mHandle(handle), mServer(server) {
    ESP_LOGD(TAG, "MeshmeshRawImpl::MeshmeshRawImpl from %ld handle %d", from, handle);
    mConnectedPath = meshmesh::global_meshmesh_component->getNetwork()->getConnectedPath();
    if (!mServer) {
      mConnectedPath->setReceiveCallback(
          [](void *arg, const uint8_t *data, uint16_t size, uint8_t connid) {
            auto a_this = (MeshmeshRawImpl *) arg;
            a_this->onRecv(data, size);
          },
          [](void *arg) {
            auto a_this = (MeshmeshRawImpl *) arg;
            a_this->onDisconnect();
          },
          this, from, handle);
    }
  }

  ~MeshmeshRawImpl() override {
    ESP_LOGD(TAG, "MeshmeshRawImpl::~MeshmeshRawImpl from %ld handle %d", mFrom, mHandle);
    if (!mServer && mActive)
      close();
  }

  void init() { ESP_LOGD(TAG, "MeshmeshRawImpl::init"); }

  std::unique_ptr<Socket> accept(struct sockaddr *addr, socklen_t *addrlen) override {
    if (mAcceptedSockets.empty()) {
      errno = EWOULDBLOCK;
      return nullptr;
    }

    std::unique_ptr<MeshmeshRawImpl> sock = std::move(mAcceptedSockets.front());
    mAcceptedSockets.pop();
    if (addr != nullptr)
      sock->getpeername(addr, addrlen);

    ESP_LOGD(TAG, "MeshmeshRawImpl::accept(%p)", sock.get());
    return std::unique_ptr<Socket>(std::move(sock));
  }

  int bind(const struct sockaddr *name, socklen_t addrlen) override {
    ESP_LOGD(TAG, "MeshmeshRawImpl::bind");
    auto family = name->sa_family;
    if (family != AF_INET) {
      errno = EINVAL;
      return -1;
    }

    auto *addr4 = reinterpret_cast<const sockaddr_in *>(name);
    mPort = ntohs(addr4->sin_port);

    ESP_LOGD(TAG, "MeshmeshRawImpl::bind(port=%u)", mPort);
    mConnectedPath->bindPort(
        [](void *s, uint32_t from, uint16_t handle) { ((MeshmeshRawImpl *) s)->onNewClient(from, handle); }, this,
        mPort);

    return 0;
  }

  int close() override {
    ESP_LOGD(TAG, "MeshmeshRawImpl::close port %u from %ld handle %d", mPort, mFrom, mHandle);
    if(mServer) {
      mConnectedPath->unbindPort(mPort);
    } else {
      mConnectedPath->closeConnection(mFrom, mHandle);
    }
    return 0;
  }

  int shutdown(int how) override {
    ESP_LOGD(TAG, "MeshmeshRawImpl::shutdown from %ld handle %d", mPort, mFrom, mHandle);
    if(mServer) {
      mConnectedPath->unbindPort(mPort);
    } else {
      mConnectedPath->closeConnection(mFrom, mHandle);
    }
    return 0;
  }

  int getpeername(struct sockaddr *name, socklen_t *addrlen) override {
    if (name == nullptr || addrlen == nullptr) {
      errno = EINVAL;
      return -1;
    }

    if (*addrlen < sizeof(struct sockaddr_in)) {
      errno = EINVAL;
      return -1;
    }

    struct sockaddr_in *addr = reinterpret_cast<struct sockaddr_in *>(name);
    addr->sin_family = AF_INET;
    *addrlen = addr->sin_len = sizeof(struct sockaddr_in);
    addr->sin_port = 0;
    addr->sin_addr.s_addr = mFrom;
    ESP_LOGD(TAG, "MeshmeshRawImpl::getpeername port %d addr %ld", addr->sin_port, addr->sin_addr.s_addr);
    return 0;
  }

  std::string getpeername() override {
    char buffer[24];
    uint32_t ip4 = mFrom;
    snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d", (uint8_t) ((ip4 >> 24) & 0xFF), (uint8_t) ((ip4 >> 16) & 0xFF),
             (uint8_t) ((ip4 >> 8) & 0xFF), (uint8_t) ((ip4 >> 0) & 0xFF));
    return std::string(buffer);
  }

  int getsockname(struct sockaddr *name, socklen_t *addrlen) override { return 0; }

  std::string getsockname() override { return std::string("*"); }

  int getsockopt(int level, int optname, void *optval, socklen_t *optlen) override {
    ESP_LOGD(TAG, "MeshmeshRawImpl::getsockopt(level=%d,optname=%d)", level, optname);
    return 0;
  }

  int setsockopt(int level, int optname, const void *optval, socklen_t optlen) override {
    ESP_LOGD(TAG, "MeshmeshRawImpl::setsockopt(level=%d,optname=%d)", level, optname);
    return 0;
  }

  int listen(int backlog) override {
    ESP_LOGD(TAG, "MeshmeshRawImpl::listen(backlog=%d)", backlog);
    return 0;
  }

  ssize_t read(void *buf, size_t len) override {
    if (!mActive) {
      errno = ENOENT;
      return -1;
    }

    if (len == 0)
      return 0;

    if (mRxQueue.size() < len) {
      errno = EWOULDBLOCK;
      return -1;
    }

    uint8_t *buf8 = reinterpret_cast<uint8_t *>(buf);
    for (int i = 0; i < len; i++) {
      *buf8++ = mRxQueue.front();
      mRxQueue.pop();
    }

    return len;
  }

  ssize_t readv(const struct iovec *iov, int iovcnt) override {
    ssize_t ret = 0;
    for (int i = 0; i < iovcnt; i++) {
      ssize_t err = read(reinterpret_cast<uint8_t *>(iov[i].iov_base), iov[i].iov_len);
      if (err == -1) {
        if (ret != 0)
          // if we already read some don't return an error
          break;
        return err;
      }
      ret += err;
      if (err != iov[i].iov_len)
        break;
    }
    return ret;
  }

  ssize_t write(const void *buf, size_t len) override {
    mConnectedPath->enqueueRadioDataToSource((const uint8_t *) buf, (uint16_t) len, mFrom, mHandle);
    return len;
  }

  ssize_t writev(const struct iovec *iov, int iovcnt) override {
    ssize_t written = 0;
    for (int i = 0; i < iovcnt; i++) {
      mConnectedPath->enqueueRadioDataToSource(reinterpret_cast<uint8_t *>(iov[i].iov_base), iov[i].iov_len, mFrom,
                                               mHandle);
      written += iov[i].iov_len;
    }
    return written;
  }

  ssize_t sendto(const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) override {
    // return ::sendto(fd_, buf, len, flags, to, tolen);
    return 0;
  }

  int setblocking(bool blocking) override {
    if (blocking) {
      // blocking operation not supported
      errno = EINVAL;
      return -1;
    }
    return 0;
  }

 protected:
  void onNewClient(uint32_t from, uint16_t handle) {
    ESP_LOGD(TAG, "MeshmeshRawImpl::onNewClient(from=%ld handle=%d)", from, handle);
    auto sock = make_unique<MeshmeshRawImpl>(from, handle, false);
    sock->init();
    mAcceptedSockets.push(std::move(sock));
  }

  void onRecv(const uint8_t *data, uint16_t size) {
    // ESP_LOGD(TAG, "MeshmeshRawImpl::onRecv(size=%d)", size);
    if (mRxQueue.size() + size > MAX_RX_QUEUE_SIZE) {
      ESP_LOGE(TAG, "MeshmeshRawImpl::onRecv(size=%d) queue of size %d is full", size, mRxQueue.size());
      size = MAX_RX_QUEUE_SIZE - mRxQueue.size();
    }
    for (int i = 0; i < size; i++)
      mRxQueue.push(data[i]);
  }

  void onDisconnect() {
    ESP_LOGD(TAG, "MeshmeshRawImpl::onDisconnect()");
    mActive = false;
  }

  uint32_t mFrom{0};
  uint16_t mHandle{0};
  in_port_t mPort{0};
  bool mServer{false};
  bool mActive{true};
  espmeshmesh::ConnectedPath *mConnectedPath{nullptr};
  std::queue<std::unique_ptr<MeshmeshRawImpl>> mAcceptedSockets;
  std::queue<uint8_t> mRxQueue;
};

std::unique_ptr<Socket> socket(int domain, int type, int protocol) {
  ESP_LOGD(TAG, "domain %d type %d, protocol %d", domain, type, protocol);
  auto *sock = new MeshmeshRawImpl(0, 0, true);  // NOLINT(cppcoreguidelines-owning-memory)
  sock->init();
  return std::unique_ptr<Socket>{sock};
}

std::unique_ptr<Socket> socket_loop_monitored(int domain, int type, int protocol) {
  // MeshmeshRawImpl doesn't use file descriptors, so monitoring is not applicable
  return socket(domain, type, protocol);
}

}  // namespace socket
}  // namespace esphome

#endif  // USE_SOCKET_IMPL_LWIP_TCP
