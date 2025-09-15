#pragma once

#include "Mod.h"

template <std::size_t T>
class ModWest81 : public Mod<T> {
public:
  constexpr explicit ModWest81(std::array<std::pair<const char*, const char*>, T> archives) :
    Mod<T>(archives, "2897299509") {}
};

namespace mods {
  static constexpr ModWest81 west81{std::array
      {std::pair{"engine.pak", "properties/resupply_hotmod.inc"}}};
}
