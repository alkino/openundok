#include <iostream>
#include <print>

#include <openundok/client.hpp>

void show_error(Result<void> result) {
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
}

template <typename T, typename F>
void with_value(Result<T> r, F&& f) {
  if (r) {
    std::forward<F>(f)(std::move(*r));
  } else {
    std::println(std::cerr, "{}", r.error());
  }
}

int main() {
  Client client("192.168.1.12");
  with_value(client.is_on(), [&](auto on) {
      std::println("{}", on);
    if (!on) {
      show_error(client.power(true));
    }
  });

  with_value(client.volume(), [&](auto vol) {
      std::println("Volume: {}", vol);
    if (vol < 8) {
      show_error(client.volume(vol + 1));
    } else {
      show_error(client.volume(6));
    }
  });

  with_value(client.artist(), [&](auto artist) {
      std::println("{}", artist);
  });

  if (auto presets = client.presets(); presets) {
    auto list = *presets;
    for (auto i: list) {
      std::string line{};
      line = std::format("[{}]: \n", i.key);
      for (auto [k, v]: i.fields) {
        line += std::format("  '{}': '{}',\n", k, to_string(v)); 
      }
      std::println("{}", line);
    }
  } else {
    std::cerr << presets.error() << std::endl;
  }

  show_error(client.select_preset(4));

  with_value(client.eqPresets(), [&](auto presets) {
      for (auto& [k, v]: presets) {
        std::println("[{}]: {}", k, v);
      }
  });

  if (auto infos = client.infos(); infos) {
    auto list = *infos;
    for (auto& [k, v]: list) {
      std::println("{}: {}", k, to_string(v));
    }
  }

  return 0;
}

// vim: set ts=2 sw=2 sts=2 et:
