#pragma once

#include "Mod.h"

/**
 * MACE
 * https://steamcommunity.com/sharedfiles/filedetails/?id=2905667604
 */
template <std::size_t T>
class ModMace : public Mod<T> {
public:
  constexpr explicit ModMace(std::array<Archive, T> archives) : Mod<T>(archives, "2905667604") {}
};

namespace mods {
  static constexpr ModMace mace{
    std::array{
      Archive{"properties.pak", "properties/resupply.inc"}
    }
  };
}
