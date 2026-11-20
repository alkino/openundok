#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "node.hpp"

class Parser {
  public:
    Parser() {}

    Result<Value> parse_answer(const std::string& body, NodeType type);
  private:
    Result<Value> parse_value(NodeType type, std::string_view text);

    template <typename T>
    Result<Value> parse_number(std::string_view text);
};

// vim: set ts=2 sw=2 sts=2 et:
