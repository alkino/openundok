#pragma once

#include <string>

#include "transport.hpp"
#include "node.hpp"

class Client {
  public:
    Client(const std::string& ip)
      : transport(ip)
    {}

    template <typename T>
    Result<T> as(Value&& v) {
      if (auto* p = std::get_if<T>(&v)) {
        return std::move(*p);
      }
      return std::unexpected("Unexpected value type");
    }

    Result<bool> is_on() {
      return transport.get(NETREMOTE_SYS_POWER)
        .and_then([this](Value&& v) { return as<uint8_t>(std::move(v)); })
        .transform([](uint8_t v){return v != 0;});
    }

    Result<void> power(bool on) {
      return transport.set(NETREMOTE_SYS_POWER, on ? "1" : "0");
    }

  private:
    Transport transport;
};

// vim: set ts=2 sw=2 sts=2 et:
