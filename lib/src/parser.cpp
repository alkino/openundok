#include <charconv>
#include <format>
#include <utility>

#include <pugixml.hpp>

#include "parser.hpp"

constexpr std::array<std::string_view, 4> xml_tag = {"u8", "u32", "s32", "c8_array"};

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
    return parse_scalar(type, child);
  }

  std::string parse_node(pugi::xml_node node) {
    return node.child_value("node");
  }

  Result<Scalar> parse_scalar(NodeType type, pugi::xml_node node) {
    const std::string_view name = node.name();

    const std::string_view expected = xml_tag[std::to_underlying(type)];
    if (name != expected) {
        return std::unexpected(std::format("type is invalid should be '{}' but is '{}'", expected, name));
    }

    auto wrap = [](auto x){ return Scalar{std::move(x)}; };

    auto value = node.child_value();

    switch(type) {
      case NodeType::U8: return parse_number<uint8_t>(value).transform(wrap);
      case NodeType::U32: return parse_number<uint32_t>(value).transform(wrap);
      case NodeType::S32: return parse_number<int32_t>(value).transform(wrap);
      case NodeType::C8_ARRAY: return Scalar{std::string(value)};
    }
    return std::unexpected("the type is not supported");
  }

  Result<std::pair<std::string, Scalar>> parse_field(pugi::xml_node field, const ListNode& node) {
    std::string name = field.attribute("name").value();

    auto it = std::ranges::find(node.fields, name, &Field::name);
    if (it == node.fields.end()) {
      return std::unexpected(std::format("the field '{}' has not been found", name)); 
    }

    auto value = parse_scalar(it->type, field.first_child());
    if (!value) {
      return std::unexpected(std::format("error with field '{}': {}", name, std::move(value.error())));
    }

    return std::make_pair(std::move(name), std::move(*value));
  }

  Result<Item> parse_item(pugi::xml_node item, const ListNode& node) {
    uint32_t key = item.attribute("key").as_uint();
    std::unordered_map<std::string, Scalar> fields;
    for (pugi::xml_node field = item.child("field"); field; field = field.next_sibling("field")) {
      if (auto res = parse_field(field, node); res) {
        fields.insert(std::move(*res));
      } else {
        return std::unexpected(std::move(res.error()));
      }
    }
    if (fields.size() != node.fields.size()) {
      return std::unexpected(std::format("item {}: expected {} fields, got {}",
            key, node.fields.size(), fields.size()));
    }
    return Item{key, std::move(fields)};
  }

  Result<List> parse_list(ListId id, pugi::xml_node xml) {
    const auto& node = list_nodes.at(id);
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

  Result<std::vector<Result<Scalar>>> parse_multiple(std::span<const ScalarId> ids) {
    auto response = doc.child("fsapiGetMultipleResponse");
    if (response.empty()) {
      return std::unexpected("the XML node <fsapiGetMultipleResponse> has not been found");
    }
    std::vector<Result<Scalar>> values;
    values.reserve(ids.size());
    for (ScalarId id: ids) {
      const auto& node = scalar_nodes[id];
      auto res = response.find_child([&](pugi::xml_node n) {
        return std::string_view(n.name()) == "fsapiResponse" && n.child_value("node") == node.name;
      });
      if (res.empty()) {
        values.push_back(std::unexpected(std::format("'{}' missing from the response", node.name)));
        continue;
      }
      values.push_back(parse_status(res)
          .and_then([&] { return parse_value(node.type, res); })
          .transform_error([&](std::string&& err) { return std::format("Error with '{}': {}", node.name, err); }));
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

Result<List> Parser::parse_list(ListId id) {
  auto response = impl_->doc.child("fsapiResponse");
  if (response.empty()) {
    return std::unexpected("the XML node <fsapiResponse> has not been found");
  }
  return impl_->parse_status(response)
    .and_then([&]{ return impl_->parse_list(id, response); });
}

Result<Page> Parser::parse_page(ListId id) {
  auto response = impl_->doc.child("fsapiResponse");
  if (response.empty()) {
    return std::unexpected("the XML node <fsapiResponse> has not been found");
  }
  // TODO: see if we can add this to parse_status directly
  if (std::string_view(response.child_value("status")) == "FS_LIST_END") {
    return Page({}, true);
  }
  return impl_->parse_status(response)
    .and_then([&] { return impl_->parse_list(id, response); })
    .transform([&](List&& items) {
      return Page{std::move(items), !response.child("listend").empty()};
  });
}

Result<std::vector<Result<Scalar>>> Parser::parse_multiple(std::span<const ScalarId> ids) {
  return impl_->parse_multiple(ids);
}

// vim: set ts=2 sw=2 sts=2 et:
