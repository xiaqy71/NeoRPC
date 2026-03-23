#include "EventLoop.hpp"
#include "TcpServer.h"

int main(int argc, char* argv[]) {
  std::shared_ptr<EventLoop> el = std::make_shared<EventLoop>();
  TcpServer server(el);
  auto s = server.start();

  el->poll();
}
