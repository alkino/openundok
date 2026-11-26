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

    Result<std::string> get(ScalarId nodeId);
    Result<std::string> set(ScalarId nodeId, const std::string& value);
    Result<std::string> list_get_next(ListId nodeId, size_t length = 30, std::optional<uint32_t> start = {});
    Result<std::string> get_multiple(std::span<const ScalarId> ids);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// vim: set ts=2 sw=2 sts=2 et:
