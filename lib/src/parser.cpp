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

  Result<void> parse_status() {
    auto response = doc.child("fsapiResponse");
    if (auto status = response.child_value("status"); status != std::string("FS_OK")) {
      return std::unexpected(status);
    }
    return {};
  }

  Result<Value> parse_value(NodeType type) {
    // TODO: check that the name is what we want <u8> <u32> ...
    auto response = doc.child("fsapiResponse");
    auto value = response.child("value").first_child().child_value();

    switch(type) {
      case NodeType::U8: return parse_number<uint8_t>(value);
      case NodeType::U32: return parse_number<uint32_t>(value);
      case NodeType::S32: return parse_number<int32_t>(value);
      case NodeType::C8_ARRAY: return std::string(value);
    }
    return std::unexpected("WTFBBQ");
  }

  template <typename T>
  Result<Value> parse_number(std::string_view text) {
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

Result<void> Parser::parse_status() {
  return impl_->parse_status();
}

Result<Value> Parser::parse_value(NodeType type) {
  return impl_->parse_value(type);
}

// vim: set ts=2 sw=2 sts=2 et:
