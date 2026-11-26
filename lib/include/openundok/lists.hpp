#pragma once

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include "types.hpp"

struct Field {
  std::string_view name;
  NodeType type;
};

struct ListNode {
  std::string_view name;
  std::span<const Field> fields = {};
};

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

struct Item {
  uint32_t key;
  std::unordered_map<std::string, Scalar> fields;
};
using List = std::vector<Item>;
struct Page { List items; bool end; };

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
