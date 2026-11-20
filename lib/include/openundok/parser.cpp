#include <charconv>

#include <pugixml.hpp>

#include "parser.hpp"

Result<Value> Parser::parse_answer(const std::string& body, NodeType type) {
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_string(body.c_str());
  if (!result) {
    return std::unexpected("XML: error when parsing xml");
  }

  auto response = doc.child("fsapiResponse");
  if (auto status = response.child_value("status"); status != std::string("FS_OK")) {
    return std::unexpected(status);
  }

  // TODO: check that the name is what we want <u8> <u32> ...
  auto value = response.child("value").first_child().child_value();

  return parse_value(type, value);
}

Result<Value> Parser::parse_value(NodeType type, std::string_view text) {
  switch(type) {
    case NodeType::U8: return parse_number<uint8_t>(text);
    case NodeType::U32: return parse_number<uint32_t>(text);
    case NodeType::S32: return parse_number<int32_t>(text);
    case NodeType::C8_ARRAY: return std::string(text);
  }
  return std::unexpected("WTFBBQ");
}

template <typename T>
Result<Value> Parser::parse_number(std::string_view text) {
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

// vim: set ts=2 sw=2 sts=2 et:
