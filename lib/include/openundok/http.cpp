#include <httplib.h>

#include "http.hpp"

struct HTTP::Impl {
  httplib::Client cli;
  const std::string pin{"1234"};

  Impl(const std::string& host)
    : cli(host)
  {}

  Result<std::string> get(std::string action, const std::string& value = {}) {
  httplib::Params final_params({{"pin", pin}});
  if (!value.empty()) {
    final_params.insert({"value", value});
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

void HTTP::set(NodeId nodeId, const std::string& value) {
  const Node& node = nodes[nodeId];
  impl_->get(std::format("/fsapi/SET/{}", node.name), value);
}


// vim: set ts=2 sw=2 sts=2 et:
