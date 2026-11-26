#include "client.hpp"

#include <concepts>

namespace {
  template <typename T> struct choice;

  template <typename T> struct choice;

  template <typename T>
  concept Choice = requires(const Item& item) {
    { choice<T>::current }   -> std::convertible_to<ScalarId>;
    { choice<T>::list }      -> std::convertible_to<ListId>;
    { choice<T>::from(item) } -> std::same_as<T>;
    requires std::unsigned_integral<node_t<choice<T>::current>>;
    requires std::same_as<decltype(T::slot), uint32_t>;
  };

  template <> struct choice<Mode> {
    static constexpr ScalarId current = NETREMOTE_SYS_MODE;
    static constexpr ListId   list    = NETREMOTE_SYS_CAPS_VALIDMODES;

    static Mode from(const Item& item) {
      ItemView<list> v{item};
      return {
        .slot = item.key,
        .id = v.get<"id">(),
        .label = v.get<"label">(),
        .modetype = v.get<"modetype">(),
        .streamable = v.get<"streamable">() != 0,
        .selectable = v.get<"selectable">() != 0,
      };
    }
  };
  static_assert(Choice<Mode>);

  template <> struct choice<Lang> {
    static constexpr ScalarId current = NETREMOTE_SYS_LANG;
    static constexpr ListId list = NETREMOTE_SYS_CAPS_VALIDLANG;

    static Lang from(const Item& item) {
      ItemView<list> v{item};
      return {
        .slot = item.key,
        .label = v.get<"langlabel">(),
      };
    }
  };
  static_assert(Choice<Lang>);

  template <> struct choice<EqPreset> {
    static constexpr ScalarId current = NETREMOTE_SYS_AUDIO_EQPRESET;
    static constexpr ListId list = NETREMOTE_SYS_CAPS_EQPRESETS;

    static EqPreset from(const Item& item) {
      ItemView<list> v{item};
      return {
        .slot = item.key,
        .label = v.get<"label">(),
      };
    }
  };
  static_assert(Choice<EqPreset>);
}

template <typename T>
Result<std::vector<T>> Client::all() {
  std::vector<T> vec;
  for (auto&& r: transport.list<choice<T>::list>()) {
    if (!r) return std::unexpected(std::move(r.error()));
    vec.push_back(choice<T>::from(*r));
  }
  return vec;
}

template <typename T>
Result<T> Client::current() {
  return transport.get<choice<T>::current>()
    .and_then([&](uint32_t cur) {
      return transport.list_get<choice<T>::list>(cur)
        .transform([](Item&& item) {
          return choice<T>::from(item);
        });
    });
}

template <typename T>
Result<void> Client::select(uint32_t slot) {
  return transport.set<choice<T>::current>(slot);
}

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
  for (auto&& r: transport.list<NETREMOTE_NAV_PRESETS>()) {
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

Result<std::vector<EqPreset>> Client::eq_presets() {
  return all<EqPreset>();
}

Result<uint8_t> Client::eq_preset() {
  return transport.get<NETREMOTE_SYS_AUDIO_EQPRESET>();
}

Result<void> Client::eq_preset(uint32_t slot) {
  return select<EqPreset>(slot);
}

Result<EqPreset> Client::current_eq_preset() {
  return current<EqPreset>();
}

Result<std::vector<Mode>> Client::modes() {
  return all<Mode>();
}

Result<uint32_t> Client::mode() {
  return transport.get<NETREMOTE_SYS_MODE>();
}

Result<void> Client::mode(uint32_t value) {
  return select<Mode>(value);
}

Result<Mode> Client::current_mode() {
  return current<Mode>();
}

Result<std::vector<Lang>> Client::langs() {
  return all<Lang>();
}

Result<uint32_t> Client::lang() {
  return transport.get<NETREMOTE_SYS_LANG>();
}

Result<void> Client::lang(uint32_t value) {
  return select<Lang>(value);
}

Result<Lang> Client::current_lang() {
  return current<Lang>();
}

Result<NowPlaying> Client::now_playing() {
  return transport.get_multiple<NETREMOTE_PLAY_INFO_ALBUM, NETREMOTE_PLAY_INFO_ARTIST,
          NETREMOTE_PLAY_INFO_DURATION, NETREMOTE_PLAY_INFO_NAME,
          NETREMOTE_PLAY_INFO_TEXT, NETREMOTE_PLAY_POSITION>()
    .transform([](auto&& t) {
      auto& [album, artist, duration, name, text, position] = t;
      return NowPlaying{
        .name = name.value_or(""),
        .artist = artist.value_or(""),
        .album = album.value_or(""),
        .text = text.value_or(""),
        .duration = std::chrono::milliseconds(duration.value_or(0)),
        .position = std::chrono::milliseconds(position.value_or(0)),
      };
    });
}

// vim: set ts=2 sw=2 sts=2 et:
