#pragma once

#include "Mod.h"

/**
 * west 81
 * https://steamcommunity.com/sharedfiles/filedetails/?id=2897299509
 */
template <std::size_t T>
class ModWest81 : public Mod<T> {
public:
  constexpr explicit ModWest81(std::array<Archive, T> archives) : Mod<T>(archives, "2897299509") {}
};

namespace mods {
  static constexpr ModWest81 west81{
    std::array{
      Archive{"engine.pak", "properties/resupply_hotmod.inc"}
    }
  };
}
