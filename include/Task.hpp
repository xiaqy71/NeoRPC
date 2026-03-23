#pragma once

#include <coroutine>
#include <utility>

template <typename T>
class Task {
 public:
  struct promise_type {
   private:
    T value;
    std::coroutine_handle<> continuation{};

   public:
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    auto initial_suspend() { return std::suspend_never(); }  // 直接启动协程
    void unhandled_exception() { std::terminate(); }
    auto final_suspend() noexcept {
      struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }
        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<promise_type> h) const noexcept {
          auto continuation = h.promise().continuation;
          if (continuation) {
            return continuation;
          }
          return std::noop_coroutine();
        }
        void await_resume() const noexcept {}
      };
      return FinalAwaiter{};
    }

    void return_value(T v) { value = v; }
    T result() const { return value; }
    void set_continuation(std::coroutine_handle<> h) noexcept {
      continuation = h;
    }
  };

 private:
 public:
  std::coroutine_handle<promise_type> handle;
  Task(auto h) : handle(h) {}
  Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
  Task(Task const&) = delete;
  Task& operator=(Task const&) = delete;
  Task& operator=(Task&& other) noexcept {
    if (this != &other) {
      if (handle) {
        handle.destroy();
      }
      handle = other.handle;
      other.handle = nullptr;
    }
    return *this;
  }

  ~Task() {
    if (handle) {
      handle.destroy();
    }
  }

  T get() { return handle.promise().result(); }

  bool await_ready() const noexcept { return handle.done(); }

  std::coroutine_handle<> await_suspend(
      std::coroutine_handle<> awaiting) noexcept {
    handle.promise().set_continuation(awaiting);
    return handle;
  }

  T await_resume() { return handle.promise().result(); }
};
