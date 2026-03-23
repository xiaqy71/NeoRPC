#pragma once

#include "UniqueFd.hpp"
#include <array>
#include <cerrno>
#include <coroutine>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <sys/epoll.h>

class EventLoop {
 private:
  struct HandlerEntry {
    uint32_t interests{0};
    void* read_handle{nullptr};   // 读事件发生时唤醒
    void* write_handle{nullptr};  // 写事件发生时唤醒
  };

  UniqueFd m_epfd;
  std::unordered_map<int, HandlerEntry> m_handlers;
  bool m_running{true};

  void updateRegistration(int fd, HandlerEntry const& entry);
  void clearReadInterest(int fd, HandlerEntry& entry);
  void clearWriteInterest(int fd, HandlerEntry& entry);

 public:
  EventLoop(/* args */) {
    int epfd = epoll_create1(0);
    if (epfd < 0) {
      throw std::runtime_error(std::string("epoll_create failed: ") +
                               std::strerror(errno));
    }
    m_epfd.reset(epfd);
  }
  ~EventLoop() = default;
  void addRead(int fd, std::coroutine_handle<> h);
  void addWrite(int fd, std::coroutine_handle<> h);
  void remove(int fd);
  void stop() noexcept { m_running = false; }
  void poll();
};

inline void EventLoop::updateRegistration(int fd, HandlerEntry const& entry) {
  if (entry.interests == 0) {
    if (epoll_ctl(m_epfd.get(), EPOLL_CTL_DEL, fd, nullptr) < 0 &&
        errno != ENOENT && errno != EBADF) {
      throw std::runtime_error(std::string("epoll_ctl del failed: ") +
                               std::strerror(errno));
    }
    return;
  }

  epoll_event ev{};
  ev.data.fd = fd;
  ev.events = entry.interests | EPOLLERR | EPOLLHUP;

  int op = EPOLL_CTL_MOD;
  if (!m_handlers.contains(fd)) {
    op = EPOLL_CTL_ADD;
  }

  if (epoll_ctl(m_epfd.get(), op, fd, &ev) == 0) {
    return;
  }

  if (op == EPOLL_CTL_MOD && errno == ENOENT) {
    if (epoll_ctl(m_epfd.get(), EPOLL_CTL_ADD, fd, &ev) == 0) {
      return;
    }
  }

  throw std::runtime_error(std::string("epoll_ctl update failed: ") +
                           std::strerror(errno));
}

inline void EventLoop::addRead(int fd, std::coroutine_handle<> h) {
  HandlerEntry entry{};
  auto it = m_handlers.find(fd);
  if (it != m_handlers.end()) {
    entry = it->second;
  }

  entry.interests |= EPOLLIN;
  entry.read_handle = h.address();
  updateRegistration(fd, entry);
  m_handlers[fd] = entry;
}

inline void EventLoop::addWrite(int fd, std::coroutine_handle<> h) {
  HandlerEntry entry{};
  auto it = m_handlers.find(fd);
  if (it != m_handlers.end()) {
    entry = it->second;
  }

  entry.interests |= EPOLLOUT;
  entry.write_handle = h.address();
  updateRegistration(fd, entry);
  m_handlers[fd] = entry;
}

inline void EventLoop::clearReadInterest(int fd, HandlerEntry& entry) {
  entry.read_handle = nullptr;
  entry.interests &= ~EPOLLIN;
  if (entry.interests == 0) {
    remove(fd);
    return;
  }
  updateRegistration(fd, entry);
}

inline void EventLoop::clearWriteInterest(int fd, HandlerEntry& entry) {
  entry.write_handle = nullptr;
  entry.interests &= ~EPOLLOUT;
  if (entry.interests == 0) {
    remove(fd);
    return;
  }
  updateRegistration(fd, entry);
}

inline void EventLoop::remove(int fd) {
  auto it = m_handlers.find(fd);
  if (it == m_handlers.end()) {
    return;
  }
  if (epoll_ctl(m_epfd.get(), EPOLL_CTL_DEL, fd, nullptr) < 0 &&
      errno != ENOENT && errno != EBADF) {
    throw std::runtime_error(std::string("epoll_ctl del failed: ") +
                             std::strerror(errno));
  }
  m_handlers.erase(it);
}

inline void EventLoop::poll() {
  std::array<epoll_event, 64> evs{};
  while (m_running) {
    int nfds =
        epoll_wait(m_epfd.get(), evs.data(), static_cast<int>(evs.size()), -1);
    if (nfds < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error(std::string("epoll_wait failed: ") +
                               std::strerror(errno));
    }

    for (int i = 0; i < nfds; ++i) {
      int fd = evs[i].data.fd;
      uint32_t revents = evs[i].events;

      auto it = m_handlers.find(fd);
      if (it == m_handlers.end()) {
        continue;
      }

      auto* read_address = ((revents & (EPOLLIN | EPOLLERR | EPOLLHUP)) &&
                            it->second.read_handle)
                               ? it->second.read_handle
                               : nullptr;
      auto* write_address = ((revents & (EPOLLOUT | EPOLLERR | EPOLLHUP)) &&
                             it->second.write_handle)
                                ? it->second.write_handle
                                : nullptr;

      if (read_address != nullptr) {
        clearReadInterest(fd, it->second);
        auto h = std::coroutine_handle<>::from_address(read_address);
        h.resume();
        it = m_handlers.find(fd);
      }

      if (write_address != nullptr) {
        if (it == m_handlers.end()) {
          auto h = std::coroutine_handle<>::from_address(write_address);
          h.resume();
          continue;
        }
        clearWriteInterest(fd, it->second);
        auto h = std::coroutine_handle<>::from_address(write_address);
        h.resume();
      }
    }
  }
}
