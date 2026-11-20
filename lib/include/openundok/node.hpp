#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

/*
 * netRemote.sys.info.controllerName
 * netRemote.sys.power
 * netRemote.sys.mode
 * netRemote.sys.caps.validModes
 * netRemote.sys.caps.eqPresets
 * netRemote.sys.caps.volumeSteps
 * netRemote.sys.audio.volume
 * netRemote.sys.audio.mute
 * netRemote.sys.irAutoPlayFlag
 * netRemote.sys.net.keepConnected
 * netRemote.sys.ipod.dockStatus
 * netRemote.nav.caps
 * netRemote.nav.preset.delete
 * netRemote.nav.preset.currentPreset
 * netRemote.nav.state
 * netRemote.nav.status
 * netRemote.nav.numItems
 * netRemote.nav.depth
 * netRemote.nav.list
 * netRemote.play.info.name
 * netRemote.play.info.text
 * netRemote.play.info.artist
 * netRemote.play.info.album
 * netRemote.play.info.duration
 * netRemote.play.info.graphicUri
 * netRemote.play.position
 * netRemote.play.errorStr
 * netRemote.play.status
 * netRemote.play.repeat
 * netRemote.play.shuffle
 * netRemote.play.caps
 * netRemote.play.addPresetStatus
 * netRemote.avs.hastoken
 * netRemote.cast.to
 * netRemote.cast.appName
 * netRemote.multiroom.caps.maxClient
 */

enum class NodeType { U8, U32, S32, C8_ARRAY };

struct Node {
  std::string_view name;
  NodeType type;
};
enum NodeId {
  NETREMOTE_SYS_POWER,
  NODE_COUNT,
};

constexpr std::array<Node, NODE_COUNT> nodes = {
  {"netRemote.sys.power", NodeType::U8},
};

template <NodeType T> struct NodeTraits;

template <> struct NodeTraits<NodeType::U8> { using type = uint8_t; };
template <> struct NodeTraits<NodeType::U32> { using type = uint32_t; };
template <> struct NodeTraits<NodeType::S32> { using type = int32_t; };
template <> struct NodeTraits<NodeType::C8_ARRAY> { using type = std::string; };

template <NodeType T>
using node_value_t = typename NodeTraits<T>::type;

using Value = std::variant<uint8_t, uint32_t, int32_t, std::string>;
template <typename T>
using Result = std::expected<T, std::string>;

// vim: set ts=2 sw=2 sts=2 et:
