#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

enum class NodeType { U8, U32, S32, C8_ARRAY };
constexpr std::array<std::string_view, 4> xml_tag = {"u8", "u32", "s32", "c8_array"};

template <NodeType> struct cpp_type;
template <> struct cpp_type<NodeType::U8> { using type = uint8_t; };
template <> struct cpp_type<NodeType::U32> { using type = uint32_t; };
template <> struct cpp_type<NodeType::S32> { using type = int32_t; };
template <> struct cpp_type<NodeType::C8_ARRAY> { using type = std::string; };

struct ScalarNode {
  std::string_view name;
  NodeType type;
};

struct Field {
  std::string_view name;
  NodeType type;
};

struct ListNode {
  std::string_view name;
  std::span<const Field> fields = {};
};

enum ScalarId {
  NETREMOTE_NAV_ACTION_SELECTPRESET,
  NETREMOTE_NAV_STATE,
  NETREMOTE_PLAY_INFO_ALBUM,
  NETREMOTE_PLAY_INFO_ARTIST,
  NETREMOTE_PLAY_INFO_DURATION,
  NETREMOTE_PLAY_INFO_GRAPHICURI,
  NETREMOTE_PLAY_INFO_NAME,
  NETREMOTE_PLAY_INFO_TEXT,
  NETREMOTE_PLAY_POSITION,
  NETREMOTE_PLAY_SHUFFLESTATUS,
  NETREMOTE_SYS_AUDIO_MUTE,
  NETREMOTE_SYS_AUDIO_VOLUME,
  NETREMOTE_SYS_CAPS_VOLUMESTEPS,
  NETREMOTE_SYS_LANG,
  NETREMOTE_SYS_MODE,
  NETREMOTE_SYS_POWER,
  SCALAR_COUNT,
};

constexpr auto scalar_nodes =
  std::to_array<ScalarNode> ({
    {"netRemote.nav.action.selectPreset", NodeType::U8},
    {"netRemote.nav.state", NodeType::U8},
    {"netRemote.play.info.album", NodeType::C8_ARRAY},
    {"netRemote.play.info.artist", NodeType::C8_ARRAY},
    {"netRemote.play.info.duration", NodeType::U32},
    {"netRemote.play.info.graphicUri", NodeType::C8_ARRAY},
    {"netRemote.play.info.name", NodeType::C8_ARRAY},
    {"netRemote.play.info.text", NodeType::C8_ARRAY},
    {"netRemote.play.position", NodeType::U32},
    {"netRemote.play.shuffleStatus", NodeType::U8},
    {"netRemote.sys.audio.mute", NodeType::U8},
    {"netRemote.sys.audio.volume", NodeType::U8},
    {"netRemote.sys.caps.volumeSteps", NodeType::U8},
    {"netRemote.sys.lang", NodeType::U32},
    {"netRemote.sys.mode", NodeType::U32},
    {"netRemote.sys.power", NodeType::U8},
  });
static_assert(scalar_nodes.size() == SCALAR_COUNT);

template <ScalarId id>
using node_t = cpp_type<scalar_nodes[id].type>::type;

enum ListId {
  NETREMOTE_NAV_PRESETS,
  NETREMOTE_SYS_CAPS_EQPRESETS,
  NETREMOTE_SYS_CAPS_VALIDLANG,
  NETREMOTE_SYS_CAPS_VALIDMODES,
  LIST_COUNT,
};

constexpr std::array<Field, 3> presets_fields = {{
  {"name", NodeType::C8_ARRAY},
  {"type", NodeType::C8_ARRAY},
  {"url",  NodeType::C8_ARRAY},
}};

constexpr std::array<Field, 1> eqpresets_fields = {{
  {"label", NodeType::C8_ARRAY},
}};

constexpr std::array<Field, 5> validmodes_fields = {{
  {"id", NodeType::C8_ARRAY},
  {"selectable", NodeType::U8},
  {"label", NodeType::C8_ARRAY},
  {"streamable", NodeType::U8},
  {"modetype", NodeType::U8},
}};

constexpr std::array<Field, 1> validlang_fields = {{
  {"langlabel", NodeType::C8_ARRAY},
}};

constexpr auto list_nodes =
  std::to_array<ListNode> ({
    {"netRemote.nav.presets", presets_fields},
    {"netRemote.sys.caps.eqPresets", eqpresets_fields},
    {"netRemote.sys.caps.validLang", validlang_fields},
    {"netRemote.sys.caps.validModes", validmodes_fields},
});
static_assert(list_nodes.size() == LIST_COUNT);

constexpr std::optional<ScalarNode> node_id_from_name(std::string_view name) {
  for (auto node: scalar_nodes) {
    if (node.name == name) {
      return node;
    }
  }
  return std::nullopt;
}

using Scalar = std::variant<uint8_t, uint32_t, int32_t, std::string>;
inline std::string to_string(const Scalar& s) {
    return std::visit([](const auto& x) { return std::format("{}", x); }, s);
}

struct Item {
  uint32_t key;
  std::unordered_map<std::string, Scalar> fields;
};
using List = std::vector<Item>;
struct Page { List items; bool end; };

using MultipleScalar = std::unordered_map<std::string, Scalar>;
template <typename T>
using Result = std::expected<T, std::string>;

template <std::size_t N>
struct fixed_string {
  char data[N]{};
  constexpr fixed_string(const char (&s)[N]) { std::copy_n(s, N, data); }
  constexpr std::string_view view() const { return {data, N - 1}; }
};

template <ListId id, fixed_string name>
consteval NodeType field_type() {
  constexpr auto fields = list_nodes[id].fields;
  auto it = std::ranges::find(fields, name.view(), &Field::name);
  if (it == fields.end()) {
    throw "unknown field for this list";
  }
  return it->type;
}

template <ListId id, fixed_string name>
using field_t = cpp_type<field_type<id, name>()>::type;

template <ListId id>
struct ItemView {
  const Item& item;

  template <fixed_string name>
  const field_t<id, name>& get() const {
    return std::get<field_t<id, name>>(item.fields.at(std::string(name.view())));
  }
};

// vim: set ts=2 sw=2 sts=2 et:
