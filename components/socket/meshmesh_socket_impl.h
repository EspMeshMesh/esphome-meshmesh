#pragma once
#include "esphome/core/defines.h"

#if defined(USE_SOCKET_IMPL_MESHMESH_ESP32) || defined(USE_SOCKET_IMPL_MESHMESH_ESP8266)

#include <memory>
#include <span>
#include <map>

#include "esphome/core/helpers.h"
#include "headers.h"

namespace espmeshmesh {
class MeshSocket;
}

namespace esphome::socket {

class MeshMeshSocketImpl {
 public:
  MeshMeshSocketImpl(int fd, bool monitor_loop = false);
  ~MeshMeshSocketImpl();
  MeshMeshSocketImpl(const MeshMeshSocketImpl &) = delete;
  MeshMeshSocketImpl &operator=(const MeshMeshSocketImpl &) = delete;

  int connect(const struct sockaddr *addr, socklen_t addrlen); 
  std::unique_ptr<MeshMeshSocketImpl> accept(struct sockaddr *addr, socklen_t *addrlen);
  std::unique_ptr<MeshMeshSocketImpl> accept_loop_monitored(struct sockaddr *addr, socklen_t *addrlen);

  int bind(const struct sockaddr *addr, socklen_t addrlen);
  int close();
  int shutdown(int how);

  int getpeername(struct sockaddr *addr, socklen_t *addrlen);
  int getsockname(struct sockaddr *addr, socklen_t *addrlen);

  /// Format peer address into a fixed-size buffer (no heap allocation)
  size_t getpeername_to(std::span<char, SOCKADDR_STR_LEN> buf);
  /// Format local address into a fixed-size buffer (no heap allocation)
  size_t getsockname_to(std::span<char, SOCKADDR_STR_LEN> buf);

  int getsockopt(int level, int optname, void *optval, socklen_t *optlen);
  int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
  int listen(int backlog);
  ssize_t read(void *buf, size_t len) ;
  ssize_t recvfrom(void *buf, size_t len, sockaddr *addr, socklen_t *addr_len);
  ssize_t readv(const struct iovec *iov, int iovcnt);
  ssize_t write(const void *buf, size_t len);
  ssize_t send(const void *buf, size_t len, int flags);
  ssize_t writev(const struct iovec *iov, int iovcnt);

  ssize_t sendto(const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

  int setblocking(bool blocking);
  int loop() { return 0; }

  bool ready() const;

  int get_fd() const { return this->fd_; }


protected:
  int fd_{-1};
  bool closed_{false};
  bool loop_monitored_{false};

public:
  static espmeshmesh::MeshSocket *getSocket(int fd) { return MeshMeshSocketImpl::sockets_.at(fd); }
  static int addSocket(espmeshmesh::MeshSocket *socket) { MeshMeshSocketImpl::sockets_[MeshMeshSocketImpl::last_fd_++] = socket; return MeshMeshSocketImpl::last_fd_ - 1; }
private:
  static void removeSocket(int fd) { MeshMeshSocketImpl::sockets_.erase(fd); }
private:
  static int last_fd_;
  static std::map<uint16_t, espmeshmesh::MeshSocket *> sockets_;
};

}  // namespace esphome::socket

#endif  // USE_SOCKET_IMPL_MESHMESH_ESP32 || USE_SOCKET_IMPL_MESHMESH_ESP8266
