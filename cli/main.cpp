#include <optional>
#include <print>

#include <CLI/CLI.hpp>

#include <openundok/client.hpp>

template <typename T>
int report(const Result<T>& r) {
  if (!r) { std::println(stderr, "error: {}", r.error()); return 1;}
  return 0;
}

using Action = std::function<int(Client&)>;

template <typename T>
void add_choice(CLI::App& app, Action& action,
    const std::string& plural, const std::string& single,
    Result<std::vector<T>> (Client::*list)(),
    Result<T>              (Client::*current)(),
    Result<void>           (Client::*select)(uint32_t)) {
  app.add_subcommand(plural, "List available " + plural)->callback([&action, list] {
    action = [list](Client& c) {
      auto r = (c.*list)();
      if (r) for (auto& x: *r) std::println("[{}] {}", x.slot, x.label);
      return report(r);
    };
  });

  auto slot = std::make_shared<std::optional<uint32_t>>();
  auto* sub = app.add_subcommand(single, "Get/Set current " + single);
  sub->add_option("slot", *slot);
  sub->callback([&action, slot, current, select] {
    action = [slot, current, select](Client& c) {
      if (!*slot) {
        auto r = (c.*current)();
        if (r) std::println("{}", r->label);
        return report(r);
      }
      return report((c.*select)(**slot));
    };
  });
}

int main(int argc, char** argv) {
  CLI::App app{"Control Frontier Silicon / UNDOK devices"};
  app.require_subcommand(1);

  std::string host;
  app.add_option("--host", host, "Decive IP");

  Action action;
  add_choice(app, action, "langs", "lang", &Client::langs, &Client::current_lang, &Client::lang);
  add_choice(app, action, "modes", "mode", &Client::modes, &Client::current_mode, &Client::mode);
  add_choice(app, action, "eq_presets", "eq_preset", &Client::eq_presets, &Client::current_eq_preset, &Client::eq_preset);

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

  auto* muted = app.add_subcommand("muted", "Is it muted");

  std::optional<std::string> mute_arg;
  auto* mute = app.add_subcommand("mute", "Enable or disable mute");
  mute->add_option("state", mute_arg)->check(CLI::IsMember({"on", "off", "toggle"}));

  auto* now_playing = app.add_subcommand("now_playing", "Showing the current information");

  CLI11_PARSE(app, argc, argv);

  Client client(host);

  if (action) return action(client);

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

  if (*now_playing) {
    auto r = client.now_playing();
    if (r) {
      auto d = std::chrono::floor<std::chrono::seconds>(r->duration);
      auto p = std::chrono::floor<std::chrono::seconds>(r->position);

      std::println("Playing {} by {}", r->name, r->artist);
      std::println("From the album {}", r->album);
      if (d > std::chrono::hours{1}) std::println("Duration is {:%T} / {:%T}", p, d);
      else std::println("Time {:%M:%S} / {:%M:%S}", p, d);
      std::println("{}", r->text);
    }
    return report(r);
  }
}

// vim: set ts=2 sw=2 sts=2 et:
