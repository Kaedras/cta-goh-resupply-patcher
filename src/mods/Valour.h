#pragma once

#include "Mod.h"

/**
 * valour
 * https://steamcommunity.com/sharedfiles/filedetails/?id=2537987794
 */
template <std::size_t T>
class ModValour : public Mod<T> {
public:
  constexpr explicit ModValour(std::array<Archive, T> archives) :
    Mod<T>(archives, "2537987794") {}
};

namespace mods {
  static constexpr ModValour valour{
      std::array{
          Archive{"britain.pak", "properties/ammo_eng.inc"}, Archive{"fra.pak", "properties/ammo_fra.inc"},
          Archive{"hun.pak", "properties/ammo_hun.inc"},
          Archive{"ita.pak", "properties/ammo_ita.inc"}, Archive{"jap.pak", "properties/ammo_jap.inc"},
          Archive{"pol.pak", "properties/ammo_pol.inc"},
          Archive{"usaf.pak", "properties/ammo_usa.inc"}, Archive{"general.pak", "properties/resupply.inc"}
      }
  };
}
