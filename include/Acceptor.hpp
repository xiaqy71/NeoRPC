#pragma once
#include "EventLoop.hpp"
#include <arpa/inet.h>
#include <coroutine>
#include <memory>

class Acceptor {
 public:
  Acceptor(int listenfd, std::shared_ptr<EventLoop> el)
      : m_listenfd(listenfd), m_el(el) {}
  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<> h) {
    m_el->addRead(m_listenfd, h);
  }
  int await_resume() { return accept(m_listenfd, nullptr, nullptr); }

 private:
  int m_listenfd;
  std::shared_ptr<EventLoop> m_el;
};
