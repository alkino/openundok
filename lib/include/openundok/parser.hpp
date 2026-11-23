#pragma once

#include <memory>
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
    Result<List> parse_list(NodeId id);
    Result<MultipleValue> parse_multiple();

  private:
    struct Impl;
    explicit Parser(std::unique_ptr<Impl> impl);

    template <typename T>
    Result<Value> parse_number(std::string_view text);

    std::unique_ptr<Impl> impl_;
};

// vim: set ts=2 sw=2 sts=2 et:
