#include <httplib.h>

#include "http.hpp"

struct HTTP::Impl {
  httplib::Client cli;
  const std::string pin{"1234"};

  Impl(const std::string& host)
    : cli(host)
  {}

  Result<std::string> get(std::string action, const std::vector<std::pair<std::string, std::string>>& values = {}) {
  httplib::Params final_params({{"pin", pin}});
  for (const auto& [k, v]: values) {
    final_params.insert({k, v});
  }

  if (auto res = cli.Get(action, final_params)) {
    if (res->status != 200) {
      return std::unexpected("Bad status");
    }
    return res->body;
  }
  return std::unexpected("Unexpected");
  }

};

HTTP::HTTP(const std::string& ip)
  : impl_(std::make_unique<Impl>(ip))
{}

HTTP::~HTTP() = default;

Result<std::string> HTTP::get(NodeId nodeId) {
  const Node& node = nodes[nodeId];
  return impl_->get(std::format("/fsapi/GET/{}", node.name));
}

Result<std::string> HTTP::set(NodeId nodeId, const std::string& value) {
  const Node& node = nodes[nodeId];
  return impl_->get(std::format("/fsapi/SET/{}", node.name), {{"value", value}});
}

Result<std::string> HTTP::list_get_next(NodeId nodeId, size_t length, ssize_t start) {
  const Node& node = nodes[nodeId];
  return impl_->get(std::format("/fsapi/LIST_GET_NEXT/{}/{}", node.name, start), {{"maxItems", std::format("{}", length)}});
}

Result<std::string> HTTP::get_multiple(std::span<const NodeId> ids) {
  std::vector<std::pair<std::string, std::string>> params{};
  for (NodeId id: ids) {
    const Node& node = nodes[id];
    params.emplace_back(std::make_pair("node", node.name));
  }
  return impl_->get("/fsapi/GET_MULTIPLE", params);
  
}

// vim: set ts=2 sw=2 sts=2 et:
