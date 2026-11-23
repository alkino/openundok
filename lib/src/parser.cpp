#include <charconv>

#include <pugixml.hpp>

#include "parser.hpp"

struct Parser::Impl {
  pugi::xml_document doc;
  
  bool load(const std::string& body) {
    pugi::xml_parse_result result = doc.load_string(body.c_str());
    if (!result) {
      return false;
    }
    return true;
  }

  Result<void> parse_status(pugi::xml_node node) {
    if (auto status = node.child_value("status"); status != std::string("FS_OK")) {
      if (std::string_view(status) == "FS_NODE_DOES_NOT_EXIST") {
        return std::unexpected("this node does not exist");
      }
      return std::unexpected(status);
    }
    return {};
  }

  Result<Scalar> parse_value(NodeType type, pugi::xml_node node) {
    auto child = node.child("value").first_child();
    return parse_scalar<Scalar>(type, child);
  }

  std::string parse_node(pugi::xml_node node) {
    return node.child_value("node");
  }

  template <typename V>
  Result<V> parse_scalar(NodeType type, pugi::xml_node node) {
    const std::string_view name = node.name();
    switch(type) {
      case NodeType::U8:
        if (name != "u8")
          return std::unexpected(std::format("type is invalid should be 'u8' but is '{}'", name));
        break;
      case NodeType::U32:
        if (name != "u32")
          return std::unexpected(std::format("type is invalid should be 'u32' but is '{}'", name));
        break;
      case NodeType::S32:
        if (name != "s32")
          return std::unexpected(std::format("type is invalid should be 's32' but is '{}'", name));
        break;
      case NodeType::C8_ARRAY:
        if (name != "c8_array")
          return std::unexpected(std::format("type is invalid should be 'c8_array' but is '{}'", name));
        break;
      case NodeType::LIST:
        return std::unexpected("found a NodeType::LIST when parsing a scalar");
    }

    auto wrap = [](auto x){ return V{std::move(x)}; };

    auto value = node.child_value();

    switch(type) {
      case NodeType::U8: return parse_number<uint8_t>(value).transform(wrap);
      case NodeType::U32: return parse_number<uint32_t>(value).transform(wrap);
      case NodeType::S32: return parse_number<int32_t>(value).transform(wrap);
      case NodeType::C8_ARRAY: return V{std::string(value)};
    }
    return std::unexpected("the type is not supported");
  }

  Result<std::pair<std::string, Scalar>> parse_field(pugi::xml_node field, const Node& node) {
    std::string name = field.attribute("name").value();

    auto it = std::ranges::find(node.fields, name, &Field::name);
    if (it == node.fields.end()) {
      return std::unexpected(std::format("the field '{}' has not been found", name)); 
    }

    auto value = parse_scalar<Scalar>(it->type, field.first_child());
    if (!value) {
      return std::unexpected(std::format("error with field '{}': {}", name, std::move(value.error())));
    }

    return std::make_pair(std::move(name), std::move(*value));
  }

  Result<Item> parse_item(pugi::xml_node item, const Node& node) {
    uint32_t key = item.attribute("key").as_uint();
    std::unordered_map<std::string, Scalar> fields;
    for (pugi::xml_node field = item.child("field"); field; field = field.next_sibling("field")) {
      if (auto res = parse_field(field, node); res) {
        fields.insert(res.value());
      } else {
        return std::unexpected(std::move(res.error()));
      }
    }
    return Item{key, fields};
  }

  Result<List> parse_list(NodeId id, pugi::xml_node xml) {
    const Node& node = nodes.at(id);
    std::vector<Item> items{};
    for (pugi::xml_node item = xml.child("item"); item; item = item.next_sibling("item")) {
      if (auto res = parse_item(item, node); res) {
        items.push_back(res.value());
      } else {
        return std::unexpected(std::move(res.error()));
      }
    }
    return items;
  }

  Result<MultipleValue> parse_multiple() {
    MultipleValue values{};
    auto response = doc.child("fsapiGetMultipleResponse");
    if (response.empty()) {
      return std::unexpected("the XML node <fsapiGetMultipleResponse> has not been found");
    }
    for (pugi::xml_node res = response.child("fsapiResponse"); res; res = res.next_sibling("fsapiResponse")) {
      if (auto status = parse_status(res); !status) {
        continue;
      }
      std::string name = parse_node(res);
      std::optional<Node> node = node_id_from_name(name);
      if (!node) {
        continue;
      }
      auto value = parse_value(node->type, res);
      if (!value) continue;
      values.emplace(name, *value);
    }
    return values;
  }

  template <typename T>
  Result<T> parse_number(std::string_view text) {
    T v{};
    const char* last = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), last, v);
    if (ec == std::errc::result_out_of_range) {
      return std::unexpected("Value out of range: " + std::string(text));
    }
    if (ec != std::errc{} || ptr != last) {
      return std::unexpected("Invalid value: " + std::string(text));
    }
    return v;
  }
};

Result<Parser> Parser::load(const std::string& body) {
  auto impl = std::make_unique<Impl>();
  if (impl->load(body)) {
    return Parser(std::move(impl));
  }
  return std::unexpected("XML: impossible to load the answer");
}

Parser::Parser(std::unique_ptr<Impl> impl)
  : impl_(std::move(impl))
{}

Parser::~Parser() = default;
Parser::Parser(Parser&&) = default;

Result<void> Parser::parse_set() {
  auto response = impl_->doc.child("fsapiResponse");
  if (response.empty()) {
    return std::unexpected("the XML node <fsapiResponse> has not been found");
  }
  return impl_->parse_status(response);
}

Result<Scalar> Parser::parse_get(NodeType type) {
  auto response = impl_->doc.child("fsapiResponse");
  if (response.empty()) {
    return std::unexpected("the XML node <fsapiResponse> has not been found");
  }
  return impl_->parse_status(response)
    .and_then([&]{ return impl_->parse_value(type, response); });
}

Result<List> Parser::parse_list(NodeId id) {
  auto response = impl_->doc.child("fsapiResponse");
  if (response.empty()) {
    return std::unexpected("the XML node <fsapiResponse> has not been found");
  }
  return impl_->parse_status(response)
    .and_then([&]{ return impl_->parse_list(id, response); });
}

Result<MultipleValue> Parser::parse_multiple() {
  return impl_->parse_multiple();
}

// vim: set ts=2 sw=2 sts=2 et:
