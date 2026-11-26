#pragma once

#include <format>
#include <generator>
#include <string>
#include <variant>

#include "http.hpp"
#include "node.hpp"
#include "parser.hpp"

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

    std::generator<Result<Item>> list(ListId id, size_t page_size = 30) {
      std::optional<uint32_t> after;
      for (;;) {
        auto page = list_get_next(id, page_size, after);
        if (!page) {
          co_yield std::unexpected(std::move(page.error()));
          co_return;
        }
        for (Item& item: page->items) {
          after = item.key;
          co_yield std::move(item);
        }
        if (page->end || page->items.empty()) co_return;
      }
    }

    template <ListId id>
    Result<Item> list_get(uint32_t slot) {
      constexpr const auto& node = list_nodes[id];
      return http.list_get(id, slot)
        .and_then(Parser::load)
        .and_then([&](Parser&& p) { return p.parse_list(id); })
        .and_then([](List&& l) -> Result<Item> {
            if (l.size() != 1) return std::unexpected(std::format("expected 1 item, got {}", l.size()));
            return std::move(l.front());
          })
        .transform_error([&](const std::string& err) { return std::format("Error with '{}': {}", node.name, err); });
    }

    Result<MultipleScalar> get_multiple(std::span<const ScalarId> ids) {
      return http.get_multiple(ids)
        .and_then(Parser::load)
        .and_then([](Parser&& p) { return p.parse_multiple(); });
    }

  private:
    HTTP http;

    Result<Page> list_get_next(ListId id, size_t length, std::optional<uint32_t> start) {
      const auto& node = list_nodes[id];
      return http.list_get_next(id, length, start)
        .and_then(Parser::load)
        .and_then([&](Parser&& p) { return p.parse_page(id); })
        .transform_error([&](const std::string& err) { return std::format("Error with '{}': {}", node.name, err); });
    }
};

// vim: set ts=2 sw=2 sts=2 et:
