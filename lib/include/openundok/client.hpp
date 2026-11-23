#pragma once

#include <string>
#include <utility>

#include "transport.hpp"
#include "node.hpp"

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

struct Preset {
  uint32_t key;
  std::string name;
  std::string type;
  std::string url;
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

    Result<bool> is_on() {
      return transport.get(NETREMOTE_SYS_POWER)
        .and_then([](Scalar&& v) { return as<uint8_t>(std::move(v)); })
        .transform([](uint8_t v){return v != 0;});
    }

    Result<void> power(bool on) {
      return transport.set(NETREMOTE_SYS_POWER, on ? "1" : "0");
    }

    Result<uint8_t> volume() {
      return transport.get(NETREMOTE_SYS_AUDIO_VOLUME)
        .and_then([](Scalar&& v) { return as<uint8_t>(std::move(v)); });
    }

    Result<void> volume(uint8_t value) {
      return transport.set(NETREMOTE_SYS_AUDIO_VOLUME, std::format("{}", value));
    }

    Result<std::string> artist() {
      return transport.get(NETREMOTE_PLAY_INFO_ARTIST)
        .and_then([](Scalar&& v) { return as<std::string>(std::move(v)); });
    }

    Result<List> presets() {
      auto nav = NavSession::open(transport);
      if (!nav) {
        return std::unexpected(std::move(nav.error()));
      }
      return transport.list_get_next(NETREMOTE_NAV_PRESETS);
    }

    Result<void> select_preset(uint8_t value) {
      auto nav = NavSession::open(transport);
      if (!nav) {
        return std::unexpected(std::move(nav.error()));
      }
      return transport.set(NETREMOTE_NAV_ACTION_SELECTPRESET, std::format("{}", value));
    }

    Result<ListList<std::string>> eqPresets() {
      return transport.list_get_next(NETREMOTE_SYS_CAPS_EQPRESETS)
        .and_then([](List&& l) -> Result<ListList<std::string>> {
            ListList<std::string> vec;
            for (auto& [k, v]: l) {
              auto it = v.find("label");
              if (it == v.end()) {
                return std::unexpected(std::format("eq preset '{}' has no 'label'", k));
              }
              auto label = as<std::string>(std::move(it->second));
              if (!label) {
                return std::unexpected(std::move(label.error()));
              }
              vec.emplace_back(k, std::move(*label));
            }
          return vec;
        });
    }

    Result<MultipleValue> infos() {
      static constexpr NodeId ids[] = {
        NETREMOTE_PLAY_INFO_ALBUM,
        NETREMOTE_PLAY_INFO_ARTIST,
        NETREMOTE_PLAY_INFO_DURATION,
        NETREMOTE_PLAY_INFO_NAME,
        NETREMOTE_PLAY_INFO_TEXT,
      };
      return transport.get_multiple(ids);
    }

  private:
    Transport transport;
};

// vim: set ts=2 sw=2 sts=2 et:
