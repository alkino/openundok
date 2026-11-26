#include "client.hpp"

template <typename T>
Result<T> field(Item& item, std::string_view name) {
  auto it = item.fields.find(std::string(name));
  if (it == item.fields.end()) {
    return std::unexpected(std::format("item '{}' has no field '{}'", item.key, name));
  }
  return as<T>(std::move(it->second));
}

Result<bool> Client::power() {
  return transport.get(NETREMOTE_SYS_POWER)
    .and_then([](Scalar&& v) { return as<uint8_t>(std::move(v)); })
    .transform([](uint8_t v){return v != 0;});
}

Result<void> Client::power(bool on) {
  return transport.set(NETREMOTE_SYS_POWER, on ? "1" : "0");
}

Result<uint8_t> Client::volume() {
  return transport.get(NETREMOTE_SYS_AUDIO_VOLUME)
    .and_then([](Scalar&& v) { return as<uint8_t>(std::move(v)); });
}

Result<void> Client::volume(uint8_t value) {
  return transport.set(NETREMOTE_SYS_AUDIO_VOLUME, std::format("{}", value));
}

Result<uint8_t> Client::volume_max() {
  return transport.get(NETREMOTE_SYS_CAPS_VOLUMESTEPS)
    .and_then([](Scalar&& v) { return as<uint8_t>(std::move(v)); })
    .transform([](uint8_t v) { return v - 1;});
}

Result<bool> Client::muted() {
  return transport.get(NETREMOTE_SYS_AUDIO_MUTE)
    .and_then([](Scalar&& v) { return as<uint8_t>(std::move(v)); })
    .transform([](uint8_t v) { return v != 0; });
}

Result<void> Client::mute(bool value) {
  return transport.set(NETREMOTE_SYS_AUDIO_MUTE, value ? "1" : "0");
}

Result<std::vector<Preset>> Client::presets() {
  auto nav = NavSession::open(transport);
  if (!nav) {
    return std::unexpected(std::move(nav.error()));
  }
  return transport.list_get_next(NETREMOTE_NAV_PRESETS)
    .and_then([](List&& list) -> Result<std::vector<Preset>> {
      std::vector<Preset> presets;
      for (auto& item: list) {
        auto name = field<std::string>(item, "name");
        auto type = field<std::string>(item, "type");
        auto url = field<std::string>(item, "url");
        if (!name) return std::unexpected(std::move(name.error()));
        if (!type) return std::unexpected(std::move(type.error()));
        if (!url) return std::unexpected(std::move(url.error()));
        if (name->empty()) continue;
        presets.push_back({item.key, std::move(*name), std::move(*type), std::move(*url)});
      }
      return presets;
    });
}

Result<void> Client::play_preset(uint32_t value) {
  auto nav = NavSession::open(transport);
  if (!nav) {
    return std::unexpected(std::move(nav.error()));
  }
  return transport.set(NETREMOTE_NAV_ACTION_SELECTPRESET, std::format("{}", value));
}

Result<ListList<std::string>> Client::eqPresets() {
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

Result<std::vector<Mode>> Client::modes() {
  return transport.list_get_next(NETREMOTE_SYS_CAPS_VALIDMODES)
    .and_then([](List&& list) -> Result<std::vector<Mode>> {
      std::vector<Mode> modes;
      for (auto& item: list) {
        auto id = field<std::string>(item, "id");
        auto selectable = field<uint8_t>(item, "selectable");
        auto label = field<std::string>(item, "label");
        auto streamable = field<uint8_t>(item, "streamable");
        auto modetype = field<uint8_t>(item, "modetype");
        if (!id) return std::unexpected(std::move(id.error()));
        if (!selectable) return std::unexpected(std::move(selectable.error()));
        if (!label) return std::unexpected(std::move(label.error()));
        if (!streamable) return std::unexpected(std::move(streamable.error()));
        if (!modetype) return std::unexpected(std::move(modetype.error()));
        modes.push_back({item.key, std::move(*id), std::move(*label), *modetype, *selectable == 1, *streamable == 1});
      }
      return modes;
    });
}

Result<uint32_t> Client::mode() {
  return transport.get(NETREMOTE_SYS_MODE)
    .and_then([](Scalar&& v) { return as<uint32_t>(std::move(v)); });
}

Result<void> Client::mode(uint32_t value) {
  return transport.set(NETREMOTE_SYS_MODE, std::format("{}", value));
}

Result<Mode> Client::current_mode() {
  return mode().and_then([&](uint32_t cur) {
      return modes().and_then([cur](std::vector<Mode>&& list) -> Result<Mode> {
          auto it = std::ranges::find(list, cur, &Mode::slot);
          if (it == list.end()) {
            return std::unexpected(std::format("unknown mode '{}'", cur));
          }
          return std::move(*it);
      });
  });
}

Result<MultipleValue> Client::infos() {
  static constexpr NodeId ids[] = {
    NETREMOTE_PLAY_INFO_ALBUM,
    NETREMOTE_PLAY_INFO_ARTIST,
    NETREMOTE_PLAY_INFO_DURATION,
    NETREMOTE_PLAY_INFO_NAME,
    NETREMOTE_PLAY_INFO_TEXT,
  };
  return transport.get_multiple(ids);
}

// vim: set ts=2 sw=2 sts=2 et:
