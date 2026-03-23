#pragma once

#include <atomic>
#include <string>

class StatsService {
 private:
  std::atomic<int> m_active_connections{0};
  std::atomic<int> m_total_connections{0};
  std::atomic<int> m_total_requests{0};

 public:
  void onConnectionOpened() {
    m_active_connections.fetch_add(1, std::memory_order_relaxed);
    m_total_connections.fetch_add(1, std::memory_order_relaxed);
  }

  void onConnectionClosed() {
    m_active_connections.fetch_sub(1, std::memory_order_relaxed);
  }

  void onRequestHandled() {
    m_total_requests.fetch_add(1, std::memory_order_relaxed);
  }

  int activeConnections() const {
    return m_active_connections.load(std::memory_order_relaxed);
  }

  int totalConnections() const {
    return m_total_connections.load(std::memory_order_relaxed);
  }

  int totalRequests() const {
    return m_total_requests.load(std::memory_order_relaxed);
  }

  std::string snapshot() const {
    return "active_connections=" + std::to_string(activeConnections()) +
           " total_connections=" + std::to_string(totalConnections()) +
           " total_requests=" + std::to_string(totalRequests());
  }
};
