#pragma once
#include <utility>
#include <unistd.h>

class UniqueFd {
 private:
  int m_fd{-1};

 public:
  UniqueFd() noexcept = default;
  explicit UniqueFd(int fd) noexcept : m_fd(fd) {}

  UniqueFd(UniqueFd const&) = delete;
  UniqueFd& operator=(UniqueFd const&) = delete;

  UniqueFd(UniqueFd&& other) noexcept : m_fd(std::exchange(other.m_fd, -1)) {}

  UniqueFd& operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
      reset();
      m_fd = std::exchange(other.m_fd, -1);
    }
    return *this;
  }

  ~UniqueFd() {
    if (m_fd >= 0) ::close(m_fd);
  }

  int get() const noexcept { return m_fd; }
  bool valid() const noexcept { return m_fd >= 0; }

  int release() noexcept { return std::exchange(m_fd, -1); }

  void reset(int fd = -1) noexcept {
    if (m_fd >= 0) {
      ::close(m_fd);
    }
    m_fd = fd;
  }
};