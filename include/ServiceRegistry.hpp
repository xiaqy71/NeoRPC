#pragma once

#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

class ServiceRegistry {
 private:
  std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;

 public:
  template <typename T>
  void add(std::shared_ptr<T> service) {
    m_services[std::type_index(typeid(T))] = std::move(service);
  }

  template <typename T>
  std::shared_ptr<T> get() const {
    auto it = m_services.find(std::type_index(typeid(T)));
    if (it == m_services.end()) {
      throw std::runtime_error("service not found");
    }
    return std::static_pointer_cast<T>(it->second);
  }

  template <typename T>
  bool contains() const {
    return m_services.contains(std::type_index(typeid(T)));
  }
};
