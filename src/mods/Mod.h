#pragma once

#include <cstddef>
#include <array>
#include <utility>

template <std::size_t T>
class Mod
{
public:
    constexpr Mod(std::array<std::pair<const char*, const char*>, T> archives,
                  const char* workshopID) : archives(archives), workshopID(workshopID)
    {
    }

    const std::array<std::pair<const char*, const char*>, T> archives;
    const char* const workshopID;
};
