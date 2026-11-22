#include <iostream>

#include <openundok/client.hpp>

int main() {
  Client client("192.168.1.12");
  auto is_on = client.is_on();
  if (is_on) {
    std::cout << client.is_on().value() << std::endl;
    if (!(is_on.value())) {
      if (auto result = client.power(true); !result) {
        std::cerr << result.error() << std::endl;
      }
    }
  } else {
    std::cout << is_on.error() << std::endl;
  }
  return 0;
}

// vim: set ts=2 sw=2 sts=2 et:
