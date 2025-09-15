#pragma once

#include "Mod.h"

template <std::size_t T>
class ModValour : public Mod<T>
{
public:
    constexpr explicit ModValour(std::array<std::pair<const char*, const char*>, T> archives) :
        Mod<T>(archives, "2537987794")
    {
    }
};

namespace mods
{
    static constexpr ModValour valour{
        std::array{
            std::pair{"britain.pak", "properties/ammo_eng.inc"}, std::pair{"fra.pak", "properties/ammo_fra.inc"},
            std::pair{"hun.pak", "properties/ammo_hun.inc"},
            std::pair{"ita.pak", "properties/ammo_ita.inc"}, std::pair{"jap.pak", "properties/ammo_jap.inc"},
            std::pair{"pol.pak", "properties/ammo_pol.inc"},
            std::pair{"usaf.pak", "properties/ammo_usa.inc"}, std::pair{"general.pak", "properties/resupply.inc"}
        }
    };
}
