#pragma once
#include "EventLoop.hpp"
#include "LogicHandler.hpp"
#include "ServiceRegistry.hpp"
#include "Task.hpp"
#include "UniqueFd.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

class Conn : public std::enable_shared_from_this<Conn> {
 private:
  static constexpr size_t kMaxLineLength = 4096;

  UniqueFd m_fd;
  std::shared_ptr<EventLoop> m_loop;
  std::shared_ptr<ServiceRegistry> m_services;
  LogicHandler m_logic_handler;
  std::function<void(int)> m_on_close;
  std::shared_ptr<Conn> m_self_guard;

  std::string m_inbuf;
  std::string m_outbuf;
  bool m_closed{false};
  std::optional<Task<int>> m_serve_task;

 public:
  Conn(int fd, std::shared_ptr<EventLoop> loop,
       std::shared_ptr<ServiceRegistry> services,
       std::function<void(int)> on_close = {})
      : m_fd(fd),
        m_loop(std::move(loop)),
        m_services(std::move(services)),
        m_on_close(std::move(on_close)) {}

  ~Conn() { close(); }

  int fd() const noexcept { return m_fd.get(); }
  bool closed() const noexcept { return m_closed; }

  void start();
  void close();
  Task<int> readSome(char* buf, size_t len);
  Task<int> writeSome(char const* buf, size_t len);
  Task<int> serve();
};
