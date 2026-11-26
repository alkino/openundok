#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "types.hpp"

struct ScalarNode {
  std::string_view name;
  NodeType type;
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

constexpr std::optional<ScalarNode> scalar_from_name(std::string_view name) {
  for (auto node: scalar_nodes) {
    if (node.name == name) {
      return node;
    }
  }
  return std::nullopt;
}

// TODO: remove it when we type GET_MULTIPLE
using MultipleScalar = std::unordered_map<std::string, Scalar>;

// vim: set ts=2 sw=2 sts=2 et:
