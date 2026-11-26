#pragma once

#include <algorithm>
#include <expected>
#include <string>
#include <string_view>
#include <variant>

enum class NodeType { U8, U32, S32, C8_ARRAY };

template <NodeType> struct cpp_type;
template <> struct cpp_type<NodeType::U8> { using type = uint8_t; };
template <> struct cpp_type<NodeType::U32> { using type = uint32_t; };
template <> struct cpp_type<NodeType::S32> { using type = int32_t; };
template <> struct cpp_type<NodeType::C8_ARRAY> { using type = std::string; };

using Scalar = std::variant<uint8_t, uint32_t, int32_t, std::string>;

template <typename T>
using Result = std::expected<T, std::string>;

template <std::size_t N>
struct fixed_string {
  char data[N]{};
  constexpr fixed_string(const char (&s)[N]) { std::copy_n(s, N, data); }
  constexpr std::string_view view() const { return {data, N - 1}; }
};

// vim: set ts=2 sw=2 sts=2 et:
