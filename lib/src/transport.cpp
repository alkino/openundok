#include "transport.hpp"

Transport::Transport(const std::string& ip)
: http(ip)
{}

Result<std::vector<Result<Scalar>>> Transport::get_multiple(std::span<const ScalarId> ids) {
  return http.get_multiple(ids)
    .and_then(Parser::load)
    .and_then([&](Parser&& p) { return p.parse_multiple(ids); });
}

// vim: set ts=2 sw=2 sts=2 et:
