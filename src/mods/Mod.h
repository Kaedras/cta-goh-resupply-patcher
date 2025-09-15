#pragma once

#include <cstddef>
#include <array>

struct Archive {
  constexpr Archive(const char* archive, const char* file) :
    archive(archive),
    file(file) {}

  const char* const archive; //! archive file name
  const char* const file; //! file inside the archive
};

/**
 * @brief Base class for mods
 */
template <std::size_t T>
class Mod {
public:
  constexpr Mod(std::array<Archive, T> archives,
                const char* workshopID) :
    archives(archives),
    workshopID(workshopID) {}

  const std::array<Archive, T> archives;
  const char* const workshopID;
};
