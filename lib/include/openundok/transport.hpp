#pragma once

#include <format>
#include <string>
#include <variant>

#include "http.hpp"
#include "node.hpp"
#include "parser.hpp"

/*
 * u8: uint8_t
 * u32: uint32_t
 * s32: int32_t
 * c8_array: std::string
 *
 */
class Transport {
  public:
    Transport(const std::string& ip)
    : http(ip)
    {}

    Result<Value> get(NodeId id) {
      auto body = http.get(id);
      if (!body) {
        return std::unexpected(body.error());
      }

      auto loaded = Parser::load(*body);
      if (!loaded) {
        return std::unexpected(std::move(loaded.error()));
      }
      Parser parser = std::move(*loaded);

      if (auto r = parser.parse_status(); !r) {
        return std::unexpected(std::move(r.error()));
      }

      const Node& node = nodes.at(id);
      return parser.parse_value(node.type);
    }

    Result<void> set(NodeId id, std::string v) {
      auto body = http.set(id, v);
      auto loaded = Parser::load(*body);
      if (!loaded) {
        return std::unexpected(std::move(loaded.error()));
      }
      Parser parser = std::move(*loaded);

      return parser.parse_status();
    }

  private:
    HTTP http;
};

// vim: set ts=2 sw=2 sts=2 et:
