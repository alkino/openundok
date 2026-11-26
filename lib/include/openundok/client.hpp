#pragma once

#include <string>
#include <utility>

#include "transport.hpp"
#include "node.hpp"

struct Preset { uint32_t slot; std::string name, type, url; };
struct Mode { uint32_t slot; std::string id, label; uint8_t modetype; bool streamable, selectable; };

class NavSession {
  public:
    static Result<NavSession> open(Transport& transport) {
      return transport.set(NETREMOTE_NAV_STATE, "1")
        .transform([&] { return NavSession(transport); })
        .transform_error([](const std::string& err) { return std::format("Impossible to lock a nav session: {}", err); });
    }

    NavSession(NavSession&& other) noexcept
      : transport_(std::exchange(other.transport_, nullptr))
    {}
    NavSession(const NavSession&) = delete;
    NavSession& operator=(const NavSession&) = delete;
    NavSession& operator=(NavSession&&) = delete;

    ~NavSession() {
      if (transport_) {
        (void)transport_->set(NETREMOTE_NAV_STATE, "0");
      }
    }

  private:
    explicit NavSession(Transport& transport)
      :transport_(&transport)
    {}

    Transport* transport_;
};

template <typename T>
Result<T> as(Scalar&& v) {
  if (auto* p = std::get_if<T>(&v)) {
    return std::move(*p);
  }
  return std::unexpected("Unexpected value type");
}

class Client {
  public:
    Client(const std::string& ip)
      : transport(ip)
    {}

    Result<bool> power();
    Result<void> power(bool on);

    Result<uint8_t> volume();
    Result<void> volume(uint8_t value);
    Result<uint8_t> volume_max();
    Result<bool> muted();
    Result<void> mute(bool value);

    Result<std::vector<Preset>> presets();
    Result<void> play_preset(uint32_t value);

    Result<ListList<std::string>> eqPresets();

    Result<std::vector<Mode>> modes();
    Result<uint32_t> mode();
    Result<void> mode(uint32_t);
    Result<Mode> current_mode();

    Result<MultipleValue> infos();

  private:
    Transport transport;
};

// vim: set ts=2 sw=2 sts=2 et:
