#pragma once

#include <format>
#include <string>
#include <variant>

#include "http.hpp"
#include "node.hpp"
#include "parser.hpp"

template <NodeType> struct cpp_type;
template <> struct cpp_type<NodeType::U8> { using type = uint8_t; };
template <> struct cpp_type<NodeType::U32> { using type = uint32_t; };
template <> struct cpp_type<NodeType::S32> { using type = int32_t; };
template <> struct cpp_type<NodeType::C8_ARRAY> { using type = std::string; };

template <ScalarId id>
using node_t = cpp_type<scalar_nodes[id].type>::type;

class Transport {
  public:
    explicit Transport(const std::string& ip)
    : http(ip)
    {}

    template <ScalarId id>
    Result<node_t<id>> get() {
      constexpr const auto& node = scalar_nodes[id];
      return http.get(id)
        .and_then(Parser::load)
        .and_then([&](Parser&& p) { return p.parse_get(node.type); })
        .transform([](Scalar&& v) { return std::get<node_t<id>>(std::move(v)); })
        .transform_error([&](const std::string& err) { return std::format("Error with '{}': {}", node.name, err); });
    }

    template <ScalarId id>
    Result<void> set(const node_t<id>& v) {
      constexpr const auto& node = scalar_nodes[id];
      return http.set(id, std::format("{}", v))
        .and_then(Parser::load)
        .and_then([](Parser&& p) { return p.parse_set(); })
        .transform_error([&](const std::string& err) { return std::format("Error with '{}': {}", node.name, err); });
    }

    Result<List> list_get_next(ListId id, size_t length = 30, std::optional<uint32_t> start = {}) {
      const auto& node = list_nodes.at(id);
      return http.list_get_next(id, length, start)
        .and_then(Parser::load)
        .and_then([&](Parser&& p) { return p.parse_list(id); })
        .transform_error([&](const std::string& err) { return std::format("Error with '{}': {}", node.name, err); });
    }

    Result<MultipleScalar> get_multiple(std::span<const ScalarId> ids) {
      return http.get_multiple(ids)
        .and_then(Parser::load)
        .and_then([](Parser&& p) { return p.parse_multiple(); });
    }

  private:
    HTTP http;
};

// vim: set ts=2 sw=2 sts=2 et:
