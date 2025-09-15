#pragma once

#include "Mod.h"

template <std::size_t T>
class ModHotmod : public Mod<T>
{
public:
    constexpr explicit ModHotmod(std::array<Archive, T> archives) :
        Mod<T>(archives, "2614199156")
    {
    }
};

namespace mods
{
    static constexpr ModHotmod hotmod{
        std::array{
            Archive{"gamelogic.pak", "properties/resupply_hotmod.inc"},
            Archive{"gamelogic.pak", "properties/resupply_vanilla.inc"}
        }
    };
}