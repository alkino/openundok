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
      const Node& node = nodes.at(id);

      auto body = http.get(id);
      if (!body) {
        return std::unexpected(body.error());
      }

      Parser parser{};
      return parser.parse_answer(*body, node.type);
    }

    void set(NodeId id, std::string v) {
      http.set(id, v);
    }

  private:
    HTTP http;
};

// vim: set ts=2 sw=2 sts=2 et:
