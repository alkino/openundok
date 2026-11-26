#include <optional>
#include <print>

#include <CLI/CLI.hpp>

#include <openundok/client.hpp>

template <typename T>
int report(const Result<T>& r) {
  if (!r) { std::println(stderr, "error: {}", r.error()); return 1;}
  return 0;
}

int main(int argc, char** argv) {
  CLI::App app{"Control Frontier Silicon / UNDOK devices"};
  app.require_subcommand(1);

  std::string host;
  app.add_option("--host", host, "Decive IP");

  std::optional<std::string> power_arg;
  auto* power = app.add_subcommand("power", "Get/Set power");
  power->add_option("state", power_arg)->check(CLI::IsMember({"on", "off", "toggle"}));

  std::optional<std::string> vol_arg;
  auto* volume = app.add_subcommand("volume", "Get/Set volume (N, +N, -N)");
  volume->add_option("value", vol_arg);

  auto* preset = app.add_subcommand("presets", "List presets for this mode");

  uint32_t slot{};
  auto* play = app.add_subcommand("play", "Play a preset");
  play->add_option("slot", slot)->required();

  auto* modes = app.add_subcommand("modes", "List available modes");

  std::optional<uint32_t> mode_slot{};
  auto* mode = app.add_subcommand("mode", "List current mode");
  mode->add_option("slot", mode_slot);

  auto* muted = app.add_subcommand("muted", "Is it muted");

  std::optional<std::string> mute_arg;
  auto* mute = app.add_subcommand("mute", "Enable or disable mute");
  mute->add_option("state", mute_arg)->check(CLI::IsMember({"on", "off", "toggle"}));

  CLI11_PARSE(app, argc, argv);

  Client client(host);

  if (*power) {
    if (!power_arg) {
      auto r = client.power();
      if (r) std::println("{}", *r ? "on": "off");
      return report(r);
    }
    bool target = *power_arg == "on";
    if (*power_arg == "toggle") {
      auto cur = client.power();
      if (!cur) return report(cur);
      target = !*cur;
    }
    return report(client.power(target));
  }

  if (*volume) {
    auto max = client.volume_max();
    if (!max) return report(max);
    auto cur = client.volume();
    if (!cur) return report(cur);
    if (!vol_arg) { std::println("{} / {}", *cur, *max); return 0; }

    std::string_view s = *vol_arg;
    bool relative = s.starts_with('+') || s.starts_with('-');
    if (s.starts_with('+')) s.remove_prefix(1); // from_chars refuse '+'

    int v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || ptr != s.data() + s.size()) {
      std::println(stderr, "error: invalid volume '{}'", *vol_arg);
      return 2;
    }

    int target = std::clamp(relative ? *cur + v: v, 0, int(*max));
    return report(client.volume(uint8_t(target)));
  }

  if (*preset) {
    auto r = client.presets();
    if (r) for (auto& p: *r) std::println("[{}] {}", p.slot, p.name);
    return report(r);
  }

  if (*play) return report(client.play_preset(slot));

  if (*modes) {
    auto r = client.modes();
    if (r) for (auto& p: *r) std::println("[{}] {}", p.slot, p.label);
    return report(r);
  }

  if (*mode) {
    if (!mode_slot) {
      auto r = client.current_mode();
      if (r) std::println("{}", r->label);
      return report(r);
    }
    return report(client.mode(*mode_slot));
  }

  if (*muted) {
    auto r = client.muted();
    if (r) std::println("{}", *r);
    return report(r);
  }

  if (*mute) {
    if (!mute_arg) {
      auto r = client.muted();
      if (r) std::println("{}", *r);
      return report(r);
    }
    bool target = mute_arg == "on";
    if (mute_arg == "toggle") {
      auto cur = client.muted();
      if (!cur) return report(cur);
      target = !*cur;
    }
    return report(client.mute(target));
  }
}

// vim: set ts=2 sw=2 sts=2 et:
