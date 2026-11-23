#pragma once

#include <format>
#include <string>
#include <variant>

#include "http.hpp"
#include "node.hpp"
#include "parser.hpp"

class Transport {
  public:
    Transport(const std::string& ip)
    : http(ip)
    {}

    Result<Scalar> get(NodeId id) {
      auto body = http.get(id);
      if (!body) {
        return std::unexpected(std::move(body.error()));
      }

      auto loaded = Parser::load(*body);
      if (!loaded) {
        return std::unexpected(std::move(loaded.error()));
      }
      Parser parser = std::move(*loaded);

      const Node& node = nodes.at(id);
      return parser.parse_get(node.type)
        .transform_error([&](const std::string& err) { return std::format("Error with '{}': {}", node.name, err); });
    }

    Result<void> set(NodeId id, std::string v) {
      auto body = http.set(id, v);
      if (!body) {
        return std::unexpected(std::move(body.error()));
      }

      auto loaded = Parser::load(*body);
      if (!loaded) {
        return std::unexpected(std::move(loaded.error()));
      }
      Parser parser = std::move(*loaded);

      return parser.parse_set();
    }

    Result<List> list_get_next(NodeId id, size_t length = 30, ssize_t start = -1) {
      auto body = http.list_get_next(id, length, start);
      if (!body) {
        return std::unexpected(std::move(body.error()));
      }

      auto loaded = Parser::load(*body);
      if (!loaded) {
        return std::unexpected(std::move(loaded.error()));
      }
      Parser parser = std::move(*loaded);

      Node node = nodes.at(id);
      return parser.parse_list(id)
        .transform_error([&](const std::string& err) { return std::format("Error with '{}': {}", node.name, err); });
    }

    Result<MultipleValue> get_multiple(std::span<const NodeId> ids) {
      auto body = http.get_multiple(ids);
      if (!body) {
        return std::unexpected(std::move(body.error()));
      }

      auto loaded = Parser::load(*body);
      if (!loaded) {
        return std::unexpected(std::move(loaded.error()));
      }
      Parser parser = std::move(*loaded);
      return parser.parse_multiple();
    }

  private:
    HTTP http;
};

// vim: set ts=2 sw=2 sts=2 et:
