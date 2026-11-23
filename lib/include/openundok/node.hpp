#pragma once

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

/*
 * netRemote.sys.info.controllerName
 * netRemote.sys.mode
 * netRemote.sys.caps.validModes
 * netRemote.sys.caps.volumeSteps
 * netRemote.sys.audio.mute
 * netRemote.sys.irAutoPlayFlag
 * netRemote.sys.net.keepConnected
 * netRemote.nav.caps
 * netRemote.nav.status
 * netRemote.nav.numItems
 * netRemote.nav.depth
 * netRemote.nav.list
 * netRemote.play.errorStr
 * netRemote.play.status
 * netRemote.play.repeat
 * netRemote.play.caps
 * netRemote.play.addPresetStatus
 */

enum class NodeType { U8, U32, S32, C8_ARRAY, LIST };

struct Field {
  std::string_view name;
  NodeType type;
};

struct Node {
  std::string_view name;
  NodeType type;
  std::span<const Field> fields = {};
};

enum NodeId {
  NETREMOTE_NAV_ACTION_SELECTPRESET,
  NETREMOTE_NAV_PRESETS,
  NETREMOTE_NAV_STATE,
  NETREMOTE_PLAY_INFO_ALBUM,
  NETREMOTE_PLAY_INFO_ARTIST,
  NETREMOTE_PLAY_INFO_DURATION,
  NETREMOTE_PLAY_INFO_GRAPHICURI,
  NETREMOTE_PLAY_INFO_NAME,
  NETREMOTE_PLAY_INFO_TEXT,
  NETREMOTE_PLAY_POSITION,
  NETREMOTE_PLAY_SHUFFLESTATUS,
  NETREMOTE_SYS_AUDIO_VOLUME,
  NETREMOTE_SYS_CAPS_EQPRESETS,
  NETREMOTE_SYS_POWER,
  NODE_COUNT,
};

constexpr std::array<Field, 3> presets_fields = {{
  {"name", NodeType::C8_ARRAY},
  {"type", NodeType::C8_ARRAY},
  {"url",  NodeType::C8_ARRAY},
}};

constexpr std::array<Field, 1> eqpresets_fields = {{
  {"label", NodeType::C8_ARRAY},
}};

constexpr auto nodes =
  std::to_array<Node> ({
    {"netRemote.nav.action.selectPreset", NodeType::U8},
    {"netRemote.nav.presets", NodeType::LIST, presets_fields},
    {"netRemote.nav.state", NodeType::U8},
    {"netRemote.play.info.album", NodeType::C8_ARRAY},
    {"netRemote.play.info.artist", NodeType::C8_ARRAY},
    {"netRemote.play.info.duration", NodeType::U32},
    {"netRemote.play.info.graphicUri", NodeType::C8_ARRAY},
    {"netRemote.play.info.name", NodeType::C8_ARRAY},
    {"netRemote.play.info.text", NodeType::C8_ARRAY},
    {"netRemote.play.position", NodeType::U32},
    {"netRemote.play.shuffleStatus", NodeType::U8},
    {"netRemote.sys.audio.volume", NodeType::U8},
    {"netRemote.sys.caps.eqPresets", NodeType::LIST, eqpresets_fields},
    {"netRemote.sys.power", NodeType::U8},
  });
static_assert(nodes.size() == NODE_COUNT);

constexpr std::optional<Node> node_id_from_name(std::string_view name) {
  for (auto node: nodes) {
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

using Value = std::variant<uint8_t, uint32_t, int32_t, std::string, List>;
using MultipleValue = std::unordered_map<std::string, Scalar>;
template <typename T>
using Result = std::expected<T, std::string>;
template <typename T>
using ListList = std::vector<std::pair<uint32_t, T>>;

// vim: set ts=2 sw=2 sts=2 et:
