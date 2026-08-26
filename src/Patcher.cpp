#include "Patcher.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <ranges>
#include <regex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vdf_parser.hpp>

#include "Item.h"
#include "Settings.h"
#include "Timer.h"
#include "constants.h"
#include "mods/Mod.h"
#include "utils.h"

using namespace std;
namespace fs = std::filesystem;

namespace {
struct itemData_t {
  const string name;
  const regex replace;
  const regex remove;
  vector<Item> items;

  itemData_t() = delete;
  itemData_t(string name, regex replace, regex remove)
      : name(std::move(name)), replace(std::move(replace)), remove(std::move(remove)) {}
};

struct resupplyData_t {
  const regex replace;
  const string replaceWith;

  resupplyData_t() = delete;
  resupplyData_t(regex replace, string replaceWith)
      : replace(std::move(replace)), replaceWith(std::move(replaceWith)) {}
};

struct replacementData_t {
  const size_t position;
  const size_t length;
  const string replacement;
};

}  // namespace

Patcher::Patcher(std::filesystem::path outputDir, bool autodetect) noexcept(false)
    : m_outputPath(std::move(outputDir)) {
  if (exists(m_outputPath)) {
    for (const auto& entry : fs::recursive_directory_iterator(m_outputPath)) {
      if (entry.is_directory()) {
        continue;
      }
      m_outputChecksums[fs::relative(entry.path(), m_outputPath)] = sha256(entry.path());
    }
  }

  if (autodetect) {
    m_gamePath     = getGamePath();
    m_workshopPath = canonical(m_gamePath / "../../workshop/content/400750");
  }
}

Patcher::~Patcher() noexcept {
  for (const auto& entry : fs::recursive_directory_iterator(m_outputPath)) {
    if (entry.is_directory()) {
      continue;
    }
    fs::path relativePath = fs::relative(entry.path(), m_outputPath);
    try {
      if (m_outputChecksums[relativePath] != sha256(entry.path())) {
        cout << "\033[33m" << "contents of " << entry.path().string() << " have changed\033[0m\n";
      }
    } catch (const runtime_error& e) {
      spdlog::warn("Error checking for changes in file {}: {}", entry.path().string(), e.what());
    }
  }
}

void Patcher::setLibraryPath(const std::filesystem::path& libraryPath) noexcept {
  m_gamePath     = getGamePath(libraryPath);
  m_workshopPath = getWorkshopPath(libraryPath);
}

void Patcher::setGamePath(const std::filesystem::path& gamePath) noexcept {
  m_gamePath = gamePath;
}

void Patcher::setWorkshopPath(const std::filesystem::path& workshopPath) noexcept {
  m_workshopPath = workshopPath;
}

void Patcher::patchVanilla() const noexcept(false) {
  patchFileFromArchive(m_gamePath / "resource/properties.pak", "properties/resupply.inc");
}

void Patcher::patchMod(const Mod& mod) const noexcept(false) {
  std::filesystem::path path = m_workshopPath / mod.workshopID / "resource";

  for (const auto& [archive, files] : mod.archives) {
    for (const auto& file : files) {
      patchFileFromArchive(path / archive, file);
    }
  }
  for (const auto& file : mod.files) {
    patchFile(path / file, m_outputPath / file);
  }
}

void Patcher::removeResupplyRestrictions(const Mod& mod) const {
  generateItemsAll(mod);
  replaceResupply(mod);
}

void Patcher::patch(std::vector<char>& data) noexcept(false) {
  Timer t(__FUNCTION__);
  spdlog::trace("patching");
  istringstream iss(data.data());
  vector<char> out;
  out.reserve(data.size());

  string line;

  while (getline(iss, line)) {
    // remove trailing '\r'
    if (line.ends_with('\r')) {
      // spdlog::trace("removing trailing \\r");
      line.pop_back();
    }

    // using strings instead of regex would be much faster,
    // but as this function only takes a few milliseconds, the practical benefit of any optimization
    // is negligible
    if (regex_search(line, regex(re::radius))) {
      // radius
      spdlog::trace("modifying radius");
      multiplyNumberInString(line, Settings::defaults::radiusMultiplier);
    } else if (regex_search(line, regex(re::resupplyPeriod))) {
      // resupply period
      spdlog::trace("modifying resupply period");
      replaceNumberInString(line, Settings::defaults::resupplyPeriod);
    } else if (regex_search(line, regex(re::regenerationPeriod))) {
      // regeneration period
      spdlog::trace("modifying regeneration period");
      replaceNumberInString(line, Settings::defaults::regenerationPeriod);
    } else if (regex_search(line, regex(re::limit))) {
      // limit
      spdlog::trace("modifying limit");
      multiplyNumberInString(line, Settings::defaults::limitMultiplier);
    } else if (regex_search(line, regex(re::limitSpecial))) {
      // limit, value is "%supply" instead of an integer
      spdlog::trace("modifying limit %supply");
      static constexpr string stringToReplace = "%supply";
      line.replace(line.find(stringToReplace), stringToReplace.size(),
                   to_string(Settings::defaults::limitFallback));
    }

    // rtrim(line);
    line.append("\r\n");
    out.append_range(line);
  }

  swap(data, out);
}

void Patcher::patchFileFromArchive(const std::filesystem::path& archiveFile,
                                   const std::filesystem::path& fileToExtract) const
    noexcept(false) {
  vector<char> data = loadFromArchive(archiveFile, fileToExtract);
  patch(data);

  fs::path targetFile = m_outputPath / fileToExtract;

  saveToFile(data, targetFile);
}

void Patcher::patchFile(const std::filesystem::path& inputFile,
                        const std::filesystem::path& outputFile) noexcept(false) {
  vector<char> data = loadFromFile(inputFile);
  patch(data);

  saveToFile(data, outputFile);
}

void Patcher::generateItemsAll(const Mod& mod) const {
  Timer t(__FUNCTION__);

  array itemData{
      itemData_t{ "items_medic_all",      re::itemsMedic,      re::itemsMedicRemove},
      itemData_t{ "items_light_all",      re::itemsLight,      re::itemsLightRemove},
      itemData_t{ "items_heavy_all",      re::itemsHeavy,      re::itemsHeavyRemove},
      itemData_t{  "items_engineer",   re::itemsEngineer,   re::itemsEngineerRemove},
      itemData_t{"items_explosives", re::itemsExplosives, re::itemsExplosivesRemove},
  };

  for (const auto& archive : mod.archives) {
    string content = readFileToString(m_outputPath / archive.files.front());

    // extract item data
    for (auto& entry : itemData) {
      auto begin = sregex_iterator(content.begin(), content.end(), entry.replace);
      auto end   = sregex_iterator();

      for (sregex_iterator i = begin; i != end; ++i) {
        string res = i->str(1);
        istringstream iss(res);
        string line;
        string condition;

        while (getline(iss, line)) {
          trim(line);
          if (line.starts_with("{item")) {
            Item item;
            istringstream itemStream(line);
            itemStream >> item;
            if (!condition.empty()) {
              item.condition = condition;
              condition.clear();
            }
            entry.items.push_back(item);
          } else if (line.starts_with('(')) {
            condition = line;
          }
        }
      }
    }
  }

  // sort
  for (auto& entry : itemData) {
    ranges::sort(entry.items, [](const Item& lhs, const Item& rhs) {
      return lhs.strings < rhs.strings;
    });
  }

  // remove duplicates
  auto duplicateComp = [](const Item& lhs, const Item& rhs) {
    return lhs.strings == rhs.strings && lhs.condition == rhs.condition;
  };
  for (auto& entry : itemData) {
    entry.items.erase(ranges::unique(entry.items, duplicateComp).begin(), entry.items.end());
  }

  // save item data
  for (auto& entry : itemData) {
    ofstream out(m_outputPath / "properties" / (entry.name + ".inc"));
    out.exceptions(ios::failbit | ios::badbit);
    out << "(define \"" << entry.name << "\"\r\n";
    for (const auto& item : entry.items) {
      out << item << "\r\n";
    }

    out << ")\r\n";
  }

  // delete items from files
  for (const auto& archive : mod.archives) {
    fs::path file = m_outputPath / archive.files.front();

    ifstream in(file);
    in.exceptions(ios::failbit | ios::badbit);
    auto size = fs::file_size(file);
    vector<char> data(size);
    in.read(data.data(), size);
    in.close();

    // create string for regex
    string content(data.begin(), data.end());

    for (const auto& entry : itemData) {
      // replace first instance
      content = regex_replace(content, entry.replace, "(include \"" + entry.name + ".inc\")",
                              regex_constants::format_first_only);
      // remove all others
      content = regex_replace(content, entry.remove, "");
    }

    // replace data
    data.clear();
    data.insert_range(data.begin(), content);

    // write data to file
    ofstream out(file);
    out.exceptions(ios::failbit | ios::badbit);
    out.write(data.data(), data.size());
  }
}

void Patcher::replaceResupply(const Mod& mod) const {
  Timer t(__FUNCTION__);

  const array resupplies{
      resupplyData_t{re::resupplyItemsLight, "(\"items_light_all\")"},
      resupplyData_t{re::resupplyItemsHeavy, "(\"items_heavy_all\")"},
      resupplyData_t{re::resupplyItemsMedic, "(\"items_medic_all\")"}
  };

  for (const auto& archive : mod.archives) {
    fs::path file      = m_outputPath / archive.files.front();
    string fileContent = readFileToString(file);

    vector<replacementData_t> replacements;

    auto begin = sregex_iterator(fileContent.begin(), fileContent.end(), re::resupply);
    auto end   = sregex_iterator();

    for (sregex_iterator i = begin; i != end; ++i) {
      string result = i->str(1);
      for (const auto& [replace, replaceWith] : resupplies) {
        // replace first instance
        result = regex_replace(result, replace, replaceWith, regex_constants::format_first_only);
        // remove all others
        result = regex_replace(result, replace, "");
      }

      // save replacement data
      replacements.push_back(
          {static_cast<size_t>(i->position(1)), static_cast<size_t>(i->length(1)), result});
    }

    // replace strings from end to beginning to not invalidate the stored offsets
    for (const auto& replacement : std::ranges::reverse_view(replacements)) {
      fileContent.replace(replacement.position, replacement.length, replacement.replacement);
    }

    // clean up empty lines
    replacements.clear();
    begin = sregex_iterator(fileContent.begin(), fileContent.end(), re::resupply);
    end   = sregex_iterator();

    for (sregex_iterator i = begin; i != end; ++i) {
      string result = i->str(1);
      istringstream iss(result);
      string line;

      string replacement;

      auto pred = [](char c) {
        return isspace(c);
      };

      while (getline(iss, line)) {
        if (!ranges::all_of(line, pred)) {
          rtrim(line);
          replacement.append(line + "\r\n");
        }
      }
      replacements.push_back(
          {static_cast<size_t>(i->position(1)), static_cast<size_t>(i->length(1)), replacement});
    }

    // replace strings from end to beginning to not invalidate the stored offsets
    for (const auto& replacement : std::ranges::reverse_view(replacements)) {
      fileContent.replace(replacement.position, replacement.length, replacement.replacement);
    }

    // save file
    saveToFile({fileContent.begin(), fileContent.end()}, file);
  }
}
