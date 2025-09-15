#pragma once

#include "Mod.h"

template <std::size_t T>
class ModHotmod : public Mod<T>
{
public:
    constexpr explicit ModHotmod(std::array<std::pair<const char*, const char*>, T> archives) :
        Mod<T>(archives, "2614199156")
    {
    }
};

namespace mods
{
    static constexpr ModHotmod hotmod{
        std::array{
            std::pair{"gamelogic.pak", "properties/resupply_hotmod.inc"},
            std::pair{"gamelogic.pak", "properties/resupply_vanilla.inc"}
        }
    };
}
