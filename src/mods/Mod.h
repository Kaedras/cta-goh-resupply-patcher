#pragma once

#include <utility>
#include <vector>

struct Archive {
  Archive(const char* archive, const char* file) : archive(archive), files(std::vector{file}) {}
  Archive(const char* archive, std::vector<const char*> files) : archive(archive), files(std::move(files)) {}
  Archive() = delete;

  const char* const archive;            //! archive file name
  const std::vector<const char*> files; //! files to patch inside the archive
};

/**
 * @brief Base class for mods
 */
class Mod {
public:
  /**
   * @param archives Files that should be patched.
   * @param workshopID Steam Workshop ID of the mod. Required to automatically detect the mod path.
   */
  Mod(std::vector<Archive> archives, const char* workshopID) : archives(std::move(archives)), workshopID(workshopID) {}

  /**
  * @param archive File that should be patched.
  * @param workshopID Steam Workshop ID of the mod. Required to automatically detect the mod path.
  */
  Mod(Archive archive, const char* workshopID) : archives(std::vector{std::move(archive)}), workshopID(workshopID) {}
  Mod() = delete;

  const std::vector<Archive> archives;
  const char* const workshopID;
};

using Archives = std::vector<Archive>;

#define MOD(name, id) \
  class Mod##name : public Mod { \
  public: \
    explicit Mod##name(Archives archives) : Mod(archives, #id) {} \
    explicit Mod##name(Archive archive) : Mod(archive, #id) {} \
  };

#define MOD_ARCHIVES(name, ...) \
  namespace mods { \
    static const Mod##name name{ \
       __VA_ARGS__ \
    };\
  }
