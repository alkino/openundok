#pragma once

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include "node.hpp"

class HTTP {
  public:
    HTTP(const std::string& ip);
    ~HTTP();

    Result<std::string> get(NodeId nodeId);
    Result<std::string> set(NodeId nodeId, const std::string& value);
    Result<std::string> list_get_next(NodeId nodeId, size_t length = 30, ssize_t start = -1);
    Result<std::string> get_multiple(std::span<const NodeId> ids);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// vim: set ts=2 sw=2 sts=2 et:
