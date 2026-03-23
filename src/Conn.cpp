#include "Conn.h"
#include "services/StatsService.hpp"

#include <cerrno>
#include <unistd.h>

namespace {

class ReadAwaiter {
 private:
  int m_fd;
  std::shared_ptr<EventLoop> m_loop;

 public:
  ReadAwaiter(int fd, std::shared_ptr<EventLoop> loop)
      : m_fd(fd), m_loop(std::move(loop)) {}

  bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<> h) const {
    m_loop->addRead(m_fd, h);
  }

  void await_resume() const noexcept {}
};

class WriteAwaiter {
 private:
  int m_fd;
  std::shared_ptr<EventLoop> m_loop;

 public:
  WriteAwaiter(int fd, std::shared_ptr<EventLoop> loop)
      : m_fd(fd), m_loop(std::move(loop)) {}

  bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<> h) const {
    m_loop->addWrite(m_fd, h);
  }

  void await_resume() const noexcept {}
};

}  // namespace

void Conn::start() {
  m_self_guard = shared_from_this();
  m_serve_task.emplace(serve());
}

void Conn::close() {
  if (m_closed) {
    return;
  }
  auto self = m_self_guard;
  m_closed = true;

  if (m_fd.valid()) {
    m_loop->remove(m_fd.get());
    const int closed_fd = m_fd.get();
    m_fd.reset();
    if (m_on_close) {
      m_on_close(closed_fd);
    }
  }
  m_self_guard.reset();
}

Task<int> Conn::readSome(char* buf, size_t len) {
  while (true) {
    int n = static_cast<int>(::read(m_fd.get(), buf, len));
    if (n > 0) {
      co_return n;
    }
    if (n == 0) {
      co_return 0;
    }

    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      co_await ReadAwaiter(m_fd.get(), m_loop);
      continue;
    }
    co_return -1;
  }
}

Task<int> Conn::writeSome(char const* buf, size_t len) {
  size_t written = 0;
  while (written < len) {
    int n = static_cast<int>(::write(m_fd.get(), buf + written, len - written));
    if (n > 0) {
      written += static_cast<size_t>(n);
      continue;
    }
    if (n == 0) {
      co_return static_cast<int>(written);
    }

    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      co_await WriteAwaiter(m_fd.get(), m_loop);
      continue;
    }
    co_return -1;
  }

  co_return static_cast<int>(written);
}

Task<int> Conn::serve() {
  while (!closed()) {
    char buf[4096];
    int n = co_await readSome(buf, sizeof(buf));
    if (n <= 0) {
      close();
      co_return n;
    }

    m_inbuf.append(buf, static_cast<size_t>(n));
    if (m_inbuf.size() > kMaxLineLength) {
      m_outbuf = "ERR line too long\n";
      int written = co_await writeSome(m_outbuf.data(), m_outbuf.size());
      close();
      if (written != static_cast<int>(m_outbuf.size())) {
        co_return -1;
      }
      co_return -1;
    }

    while (true) {
      auto pos = m_inbuf.find('\n');
      if (pos == std::string::npos) {
        break;
      }

      if (pos > kMaxLineLength) {
        m_outbuf = "ERR line too long\n";
        int written = co_await writeSome(m_outbuf.data(), m_outbuf.size());
        close();
        if (written != static_cast<int>(m_outbuf.size())) {
          co_return -1;
        }
        co_return -1;
      }

      std::string_view request(m_inbuf.data(), pos);

      CommandContext ctx{.connection_fd = fd(), .services = m_services.get()};
      if (ctx.services != nullptr &&
          ctx.services->contains<StatsService>()) {
        ctx.services->get<StatsService>()->onRequestHandled();
      }
      m_outbuf = m_logic_handler.handle(request, ctx);
      m_outbuf.push_back('\n');
      m_inbuf.erase(0, pos + 1);

      int written = co_await writeSome(m_outbuf.data(), m_outbuf.size());
      if (written != static_cast<int>(m_outbuf.size())) {
        close();
        co_return -1;
      }
    }
  }

  co_return 0;
}
