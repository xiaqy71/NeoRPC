#include "TcpServer.h"
#include "Acceptor.hpp"
#include "services/StatsService.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <stdexcept>
void TcpServer::start_listen() {
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);

  if (listenfd == -1) {
    throw std::runtime_error("socket creation failed");
  }
  int opt = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  m_listenfd.reset(listenfd);
  qiyi::setNonblock(m_listenfd.get());

  sockaddr_in addr{};  // 清零
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(m_port);

  int ret = bind(m_listenfd.get(), (sockaddr*)&addr, sizeof(addr));
  if (ret < 0) {
    throw std::runtime_error("bind addr failed");
  }
  ret = listen(m_listenfd.get(), 128);
  if (ret == -1) {
    throw std::runtime_error("listen failed");
  }
}

TcpServer::TcpServer(std::shared_ptr<EventLoop> el, size_t port)
    : is_running(true),
      m_listenfd(-1),
      m_port(port),
      m_el(std::move(el)),
      m_services(std::make_shared<ServiceRegistry>()) {
  m_services->add(std::make_shared<StatsService>());
}

Task<int> TcpServer::start() {
  // 1. 监听
  start_listen();
  while (is_running) {
    // 2. 接受连接

    // listenfd不转移所有权
    int clifd = co_await Acceptor(m_listenfd.get(), m_el);
    if (clifd < 0) {
      throw std::runtime_error("accept error");
    }
    qiyi::setNonblock(clifd);
    std::cout << "accept fd:" << clifd << std::endl;
    if (m_services->contains<StatsService>()) {
      m_services->get<StatsService>()->onConnectionOpened();
    }
    // 3. 读请求
    auto conn = std::make_shared<Conn>(
        clifd, m_el, m_services, [this](int fd) {
          m_conns.erase(fd);
          if (m_services->contains<StatsService>()) {
            m_services->get<StatsService>()->onConnectionClosed();
          }
        });
    m_conns.insert_or_assign(clifd, conn);
    conn->start();
  }
}
