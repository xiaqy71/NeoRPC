#pragma once
#include "Conn.h"
#include "EventLoop.hpp"
#include "ServiceRegistry.hpp"
#include "Task.hpp"
#include "UniqueFd.hpp"
#include <unordered_map>
#include <memory>
#define PORT 10001

class TcpServer {
 private:
  /* data */
  bool is_running;
  UniqueFd m_listenfd;
  size_t m_port;
  std::shared_ptr<EventLoop> m_el;
  std::shared_ptr<ServiceRegistry> m_services;
  void start_listen();
  std::unordered_map<int, std::shared_ptr<Conn>> m_conns;

 public:
  TcpServer(std::shared_ptr<EventLoop> el, size_t port = PORT);
  ~TcpServer() = default;
  Task<int> start();
};
