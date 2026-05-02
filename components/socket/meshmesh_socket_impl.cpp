#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "socket.h"

#if defined(USE_SOCKET_IMPL_MESHMESH_ESP32)

#include <cstring>
#include "esphome/core/application.h"

#include <espmeshmesh.h>
#include <meshsocket.h>
namespace esphome::socket {

int MeshMeshSocketImpl::last_fd_ = 0;
std::map<uint16_t, espmeshmesh::MeshSocket *> MeshMeshSocketImpl::sockets_ = {};

MeshMeshSocketImpl::MeshMeshSocketImpl(int fd, bool monitor_loop) {
  this->fd_ = fd;
  if (!monitor_loop || this->fd_ < 0) return;
  //this->loop_monitored_ = App.register_socket_fd(this->fd_);  // TODO Implement monitored sockets for Meshmesh
}

MeshMeshSocketImpl::~MeshMeshSocketImpl() {
  if (!this->closed_) this->close();
}

int MeshMeshSocketImpl::connect(const struct sockaddr *addr, socklen_t addrlen) { 
  ESP_LOGE("MeshMeshSocketImpl", "connect not implemented");
  // TODO: Implement connect
  return -1;
}

std::unique_ptr<MeshMeshSocketImpl> MeshMeshSocketImpl::accept(struct sockaddr *addr, socklen_t *addrlen) {
  ESP_LOGE("MeshMeshSocketImpl", "accept not implemented");
  // TODO: Implement accept
  return nullptr;
}

std::unique_ptr<MeshMeshSocketImpl> MeshMeshSocketImpl::accept_loop_monitored(struct sockaddr *addr, socklen_t *addrlen) {
  auto socket = this->getSocket(this->fd_);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
    return nullptr;
  }

  auto new_socket = socket->accept();
  if(new_socket == nullptr) {
    return nullptr;
  }

  int fd = MeshMeshSocketImpl::addSocket(new_socket);
  return make_unique<MeshMeshSocketImpl>(this->last_fd_ - 1, true);
}

int MeshMeshSocketImpl::bind(const struct sockaddr *addr, socklen_t addrlen) {
  espmeshmesh::EspMeshMesh *mesh = espmeshmesh::EspMeshMesh::getInstance();
  if(mesh == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "EspMeshMesh instance is not initialized");
    return -1;
  }

  const sockaddr_in *addr_in = reinterpret_cast<const sockaddr_in *>(addr);
  auto port = ntohs(addr_in->sin_port);
  if(addr_in->sin_addr.s_addr != ESPHOME_INADDR_ANY) {
    ESP_LOGE("MeshMeshSocketImpl", "bind to specific address not supported");
  }

  auto socket = this->getSocket(this->fd_);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
    return -1;
  }

  return socket->bind(port);
}

int MeshMeshSocketImpl::close() {
  if (!this->closed_) {
    // Unregister before closing to avoid dangling pointer in monitored set
    if (this->loop_monitored_) {
      //App.unregister_socket_fd(this->fd_);  // TODO Implement monitored sockets for Meshmesh
    }

    auto socket = this->getSocket(this->fd_);
    if(socket == nullptr) {
      ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
      return -1;
    }

    int ret = socket->close();
    MeshMeshSocketImpl::removeSocket(this->fd_);

    this->fd_ = -1;
    this->closed_ = true;

    delete socket;
    return ret;
  }
  return 0;
}

int MeshMeshSocketImpl::shutdown(int how) {
  ESP_LOGE("MeshMeshSocketImpl", "shutdown not implemented");
  return this->close();
}

int MeshMeshSocketImpl::getpeername(struct sockaddr *addr, socklen_t *addrlen) {
  auto socket = this->getSocket(this->fd_);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
    return -1;
  }

  sockaddr_in *addr_in = reinterpret_cast<sockaddr_in *>(addr);
  addr_in->sin_len = sizeof(sockaddr_in);
  addr_in->sin_family = AF_INET;
  addr_in->sin_port = 0;
  addr_in->sin_addr.s_addr = htons(socket->getTargetAddress());
  return 0;
}

int MeshMeshSocketImpl::getsockname(struct sockaddr *addr, socklen_t *addrlen) {
  ESP_LOGE("MeshMeshSocketImpl", "getsockname not implemented");
  // TODO: Implement getsockname
  return -1;
}

int MeshMeshSocketImpl::setblocking(bool blocking) {
  if(blocking) {
    ESP_LOGW("MeshMeshSocketImpl", "setblocking not implemented");
    return -1;
  }
  return 0;
}

size_t MeshMeshSocketImpl::getpeername_to(std::span<char, SOCKADDR_STR_LEN> buf) {
  struct sockaddr_storage storage;
  socklen_t len = sizeof(storage);
  if (this->getpeername(reinterpret_cast<struct sockaddr *>(&storage), &len) != 0) {
    buf[0] = '\0';
    return 0;
  }
  return format_sockaddr_to(reinterpret_cast<struct sockaddr *>(&storage), len, buf);
}

size_t MeshMeshSocketImpl::getsockname_to(std::span<char, SOCKADDR_STR_LEN> buf) {
  ESP_LOGE("MeshMeshSocketImpl", "getsockname_to not implemented");
  // TODO: Implement getsockname_to
  return -1;
}

int MeshMeshSocketImpl::getsockopt(int level, int optname, void *optval, socklen_t *optlen) {
  ESP_LOGW("MeshMeshSocketImpl", "getsockopt ignored level: %d, optname: %d", level, optname);
  memset(optval, 0, *optlen);
  return 0;
}

int MeshMeshSocketImpl::setsockopt(int level, int optname, const void *optval, socklen_t optlen) {
  ESP_LOGW("MeshMeshSocketImpl", "setsockopt ignored level: %d, optname: %d", level, optname);
  return 0;
}

int MeshMeshSocketImpl::listen(int backlog) {
  auto socket = getSocket(this->fd_);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
    return -1;
  }
  return socket->listen(backlog);
}

ssize_t MeshMeshSocketImpl::read(void *buf, size_t len) {
  auto socket = getSocket(this->fd_);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
    return -1;
  }
  auto received = socket->recv((uint8_t *)buf, len);
  //ESP_LOGD("MeshMeshSocketImpl", "received: %d awaited: %d", received, len);
  if(received<0) {
    if(received == espmeshmesh::MeshSocket::errBufferTooSmall) {
      errno = EWOULDBLOCK;
      return -1;
    } else if(received == espmeshmesh::MeshSocket::errIsNotConnected) {
      return 0;
    }
  }
  return received;
}

ssize_t MeshMeshSocketImpl::recvfrom(void *buf, size_t len, sockaddr *addr, socklen_t *addr_len) {
  ESP_LOGE("MeshMeshSocketImpl", "recvfrom not implemented");
  // TODO: Implement recvfrom
  return -1;
}

ssize_t MeshMeshSocketImpl::readv(const struct iovec *iov, int iovcnt) {
  ssize_t ret = 0;
  // TODO: Implement checked readv function
  ESP_LOGW("MeshMeshSocketImpl", "using uncheked readv function");
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

ssize_t MeshMeshSocketImpl::write(const void *buf, size_t len) {
  espmeshmesh::MeshSocket *socket = getSocket(this->fd_);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
    return -1;
  }
  auto sent = socket->send((uint8_t *)buf, len);
  if(sent<0) {
    return -1;
  }
  return sent;
}

ssize_t MeshMeshSocketImpl::send(const void *buf, size_t len, int flags) {
  ESP_LOGW("MeshMeshSocketImpl", "ignored flags: %d", flags);
  espmeshmesh::MeshSocket *socket = getSocket(this->fd_);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Socket %d not found", this->fd_);
    return -1;
  }
  auto sent = socket->send((uint8_t *)buf, len);
  if(sent<0) {
    return -1;
  }
  return sent;
}

ssize_t MeshMeshSocketImpl::writev(const struct iovec *iov, int iovcnt) {
  ssize_t written = 0;
  for (int i = 0; i < iovcnt; i++) {
    written += this->write(iov[i].iov_base, iov[i].iov_len);
  }
  return written;
}

ssize_t MeshMeshSocketImpl::sendto(const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
  ESP_LOGE("MeshMeshSocketImpl", "sendto not implemented");
  // TODO: Implement sendto
  return -1;
}

// Helper to create a socket with optional monitoring
static std::unique_ptr<MeshMeshSocketImpl> create_socket(int domain, int type, int protocol, bool loop_monitored = false) {
  espmeshmesh::EspMeshMesh *mesh = espmeshmesh::EspMeshMesh::getInstance();
  if(mesh == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "EspMeshMesh instance is not initialized");
    return nullptr;
  }

  espmeshmesh::MeshSocket::SocketType socketType;
  if(type == SOCK_STREAM) {
    socketType = espmeshmesh::MeshSocket::SocketType::MM_SOCK_STREAM;
  } else if (type == SOCK_DGRAM) {
    socketType = espmeshmesh::MeshSocket::SocketType::MM_SOCK_DGRAM;
  } else {
    ESP_LOGE("MeshMeshSocketImpl", "Invalid socket type: %d", type);
    return nullptr;
  }

  auto socket = new espmeshmesh::MeshSocket(socketType);
  if(socket == nullptr) {
    ESP_LOGE("MeshMeshSocketImpl", "Failed to create socket");
    return nullptr;
  }

  int fd = MeshMeshSocketImpl::addSocket(socket);
  return make_unique<MeshMeshSocketImpl>(fd, loop_monitored);
}

std::unique_ptr<Socket> socket(int domain, int type, int protocol) {
  return create_socket(domain, type, protocol, false);
}

std::unique_ptr<Socket> socket_loop_monitored(int domain, int type, int protocol) {
  return create_socket(domain, type, protocol, true);
}

}  // namespace esphome::socket

#endif  // USE_SOCKET_IMPL_BSD_SOCKETS
