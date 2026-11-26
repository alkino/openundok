#include "client.hpp"

Result<bool> Client::power() {
  return transport.get<NETREMOTE_SYS_POWER>()
    .transform([](auto v){return v != 0;});
}

Result<void> Client::power(bool on) {
  return transport.set<NETREMOTE_SYS_POWER>(on);
}

Result<uint8_t> Client::volume() {
  return transport.get<NETREMOTE_SYS_AUDIO_VOLUME>();
}

Result<void> Client::volume(uint8_t value) {
  return transport.set<NETREMOTE_SYS_AUDIO_VOLUME>(value);
}

Result<uint8_t> Client::volume_max() {
  return transport.get<NETREMOTE_SYS_CAPS_VOLUMESTEPS>()
    .transform([](auto v) { return v == 0 ? 0 : v - 1;});
}

Result<bool> Client::muted() {
  return transport.get<NETREMOTE_SYS_AUDIO_MUTE>()
    .transform([](auto v) { return v != 0; });
}

Result<void> Client::mute(bool value) {
  return transport.set<NETREMOTE_SYS_AUDIO_MUTE>(value);
}

Result<std::vector<Preset>> Client::presets() {
  auto nav = NavSession::open(transport);
  if (!nav) return std::unexpected(std::move(nav.error()));

  std::vector<Preset> presets;
  for (auto&& r: transport.list(NETREMOTE_NAV_PRESETS)) {
    if (!r) return std::unexpected(std::move(r.error()));
    ItemView<NETREMOTE_NAV_PRESETS> v{*r};
    if (v.get<"name">().empty()) continue;
    presets.push_back({r->key, v.get<"name">(), v.get<"type">(), v.get<"url">()});
  }
  return presets;
}

Result<void> Client::play_preset(uint32_t value) {
  auto nav = NavSession::open(transport);
  if (!nav) {
    return std::unexpected(std::move(nav.error()));
  }
  return transport.set<NETREMOTE_NAV_ACTION_SELECTPRESET>(value);
}

Result<std::vector<EqPreset>> Client::eqPresets() {
  std::vector<EqPreset> eqPresets;
  for (auto&& r: transport.list(NETREMOTE_SYS_CAPS_EQPRESETS)) {
    if (!r) return std::unexpected(std::move(r.error()));
    Item& item = *r;
    eqPresets.push_back({
      item.key,
      ItemView<NETREMOTE_SYS_CAPS_EQPRESETS>{item}.get<"label">()
    });
  }
  return eqPresets;
}

Result<std::vector<Mode>> Client::modes() {
  std::vector<Mode> modes;
  for (auto&& r: transport.list(NETREMOTE_SYS_CAPS_VALIDMODES)) {
    if (!r) return std::unexpected(std::move(r.error()));
    ItemView<NETREMOTE_SYS_CAPS_VALIDMODES> v{*r};
    modes.push_back({
      r->key, v.get<"id">(), v.get<"label">(), v.get<"modetype">(),
      v.get<"streamable">() == 1, v.get<"selectable">() == 1
    });
  }
  return modes;
}

Result<uint32_t> Client::mode() {
  return transport.get<NETREMOTE_SYS_MODE>();
}

Result<void> Client::mode(uint32_t value) {
  return transport.set<NETREMOTE_SYS_MODE>(value);
}

Result<Mode> Client::current_mode() {
  return mode().and_then([&](uint32_t cur) {
    return transport.list_get<NETREMOTE_SYS_CAPS_VALIDMODES>(cur)
      .transform([](Item&& item) {
        ItemView<NETREMOTE_SYS_CAPS_VALIDMODES> v{item};
        return Mode{
          item.key, v.get<"id">(), v.get<"label">(), v.get<"modetype">(),
          v.get<"streamable">() == 1, v.get<"selectable">() == 1
        };
      });
  });
}

Result<std::vector<Lang>> Client::langs() {
  std::vector<Lang> langs;
  for (auto&& r: transport.list(NETREMOTE_SYS_CAPS_VALIDLANG)) {
    if (!r) return std::unexpected(std::move(r.error()));
    Item& item = *r;
    langs.push_back({
      item.key,
      ItemView<NETREMOTE_SYS_CAPS_VALIDLANG>{item}.get<"langlabel">()
    });
  }
  return langs;
}

Result<uint32_t> Client::lang() {
  return transport.get<NETREMOTE_SYS_LANG>();
}

Result<void> Client::lang(uint32_t value) {
  return transport.set<NETREMOTE_SYS_LANG>(value);
}

Result<Lang> Client::current_lang() {
  return lang().and_then([&](uint32_t cur) {
      return transport.list_get<NETREMOTE_SYS_CAPS_VALIDLANG>(cur)
      .transform([](Item&& item) {
          return Lang{
            item.key,
            ItemView<NETREMOTE_SYS_CAPS_VALIDLANG>{item}.get<"langlabel">()
          };
      });
  });
}

Result<MultipleScalar> Client::infos() {
  static constexpr ScalarId ids[] = {
    NETREMOTE_PLAY_INFO_ALBUM,
    NETREMOTE_PLAY_INFO_ARTIST,
    NETREMOTE_PLAY_INFO_DURATION,
    NETREMOTE_PLAY_INFO_NAME,
    NETREMOTE_PLAY_INFO_TEXT,
  };
  return transport.get_multiple(ids);
}

// vim: set ts=2 sw=2 sts=2 et:
