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

    Result<std::string> get(NodeId node);
    void set(NodeId node, const std::string& value);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// vim: set ts=2 sw=2 sts=2 et:
