#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "node.hpp"

class Parser {
  public:
    ~Parser();
    Parser(Parser&&);

    static Result<Parser> load(const std::string& body);

    Result<void> parse_set();
    Result<Scalar> parse_get(NodeType type);
    Result<std::vector<Result<Scalar>>> parse_multiple(std::span<const ScalarId> ids);

    Result<List> parse_list(ListId id);
    Result<Page> parse_page(ListId id);

  private:
    struct Impl;
    explicit Parser(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

// vim: set ts=2 sw=2 sts=2 et:
