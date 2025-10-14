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
   * @param name Mod name
   * @param archives Files that should be patched.
   * @param workshopID Steam Workshop ID of the mod. Required to automatically detect the mod path.
   */
  Mod(std::string name, std::vector<Archive> archives, const char* workshopID) : name(std::move(name)), archives(std::move(archives)), workshopID(workshopID) {}

  /**
  * @param name Mod name
  * @param archive File that should be patched.
  * @param workshopID Steam Workshop ID of the mod. Required to automatically detect the mod path.
  */
  Mod(std::string name, Archive archive, const char* workshopID) : name(std::move(name)), archives(std::vector{std::move(archive)}), workshopID(workshopID) {}
  Mod() = delete;

  const std::string name;
  const std::vector<Archive> archives;
  const char* const workshopID;
};

/**
 * @brief Define a new mod
 * @param name Mod name
 * @param id Steam Workshop ID
 */
#define MOD(name, id) \
  class Mod##name : public Mod { \
  public: \
    explicit Mod##name(std::vector<Archive> archives) : Mod(#name, archives, #id) {} \
    explicit Mod##name(Archive archive) : Mod(#name, archive, #id) {} \
  };

/**
 * @brief Define mod archives containing resupply data
 * @param name Mod name
 * @param ... Archive list. Can be either archive{} or std::vector<archive>{}
 */
#define MOD_ARCHIVES(name, ...) \
  namespace mods { \
    static const Mod##name name{ \
       __VA_ARGS__ \
    };\
  }
