#include "Patcher.h"

#include <algorithm>
#include <cctype>
#include <compare>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/types.h>
#include <ranges>
#include <regex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vdf_parser.hpp>
#include <zip.h>
#include <zipconf.h>

#include "Item.h"
#include "Settings.h"
#include "Timer.h"
#include "mods/Mod.h"
#include "spdlog/fmt/bundled/base.h"
#include "spdlog/fmt/bundled/format.h"

struct zip;
struct zip_file;

using namespace std;
namespace fs = std::filesystem;

namespace {
constexpr auto APPID = "400750";

namespace re {
  // patterns for changing resupply values
  const regex radius(R"(\{radius\s*\d+)");
  const regex resupplyPeriod(R"(\{resupplyPeriod\s*\d+)");
  const regex regenerationPeriod(R"(\{regenerationPeriod\s*\d+)");
  const regex limit(R"(\{limit\s*\d+)");
  const regex limitSpecial(R"(\{limit\s*%supply)");

  const regex resupply(R"(\{resupply\r\n([\s\S]+?)\t+\}\r\n)");

  // patterns to use when replacing lines
  const regex itemsLight(R"(\(define "items_light_\w+\"\r\n([\s\S]+?)\r\n\))");
  const regex itemsHeavy(R"(\(define "items_heavy_\w+\"\r\n([\s\S]+?)\r\n\))");
  const regex itemsMedic(R"(\(define "items_medic\w*\"\r\n([\s\S]+?)\r\n\))");
  const regex itemsEngineer(R"(\(define "items_engineer"\r\n([\s\S]+?)\r\n\))");
  const regex itemsExplosives(R"(\(define "items_explosives"\r\n([\s\S]+?)\r\n\))");
  const regex resupplyItemsLight(R"(\("items_light(?!_all)\w{1,8}"\))");
  const regex resupplyItemsHeavy(R"(\("items_heavy(?!_all)\w{1,8}"\))");
  const regex resupplyItemsMedic(R"(\("items_medic(?!_all)\w{0,5}"\))");

  // patterns to use when removing lines to prevent excessive amount of empty lines
  const regex itemsLightRemove(R"(\w*\(define "items_light_\w+\"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
  const regex itemsHeavyRemove(R"(\w*\(define "items_heavy_\w+\"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
  const regex itemsMedicRemove(R"(\w*\(define "items_medic\w*\"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
  const regex itemsEngineerRemove(R"(\w*\(define "items_engineer"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
  const regex
      itemsExplosivesRemove(R"(\w*\(define "items_explosives"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
}  // namespace re

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

struct MdCtxDeleter {
  void operator()(EVP_MD_CTX* m) const {
    if (m) {
      EVP_MD_CTX_free(m);
    }
  }
};
using MdCtxPtr = std::unique_ptr<EVP_MD_CTX, MdCtxDeleter>;

struct DigestDeleter {
  void operator()(unsigned char* d) const {
    if (d) {
      OPENSSL_free(d);
    }
  }
};
using DigestPtr = std::unique_ptr<unsigned char, DigestDeleter>;
}  // namespace

Patcher::Patcher(std::filesystem::path outputDir) noexcept(false)
    : m_outputPath(std::move(outputDir)), m_gamePath(getGamePath()),
      m_workshopPath(m_gamePath / "../../workshop/content/400750") {
  if (exists(m_outputPath)) {
    for (const auto& entry : fs::recursive_directory_iterator(m_outputPath)) {
      if (entry.is_directory()) {
        continue;
      }
      m_outputChecksums[fs::relative(entry.path(), m_outputPath)] = sha256(entry.path());
    }
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

void Patcher::patchVanilla() const noexcept(false) {
  patchFile(m_gamePath / "resource/properties.pak", "properties/resupply.inc");
}

void Patcher::patchMod(const Mod& mod) const noexcept(false) {
  std::filesystem::path path = m_workshopPath / mod.workshopID / "resource";

  for (const auto& [archive, files] : mod.archives) {
    for (const auto& file : files) {
      patchFile(path / archive, file);
    }
  }
  for (const auto& file : mod.files) {
    patchFile(file);
  }
}

void Patcher::removeResupplyRestrictions(const Mod& mod) const {
  generateItemsAll(mod);
  replaceResupply(mod);
}

std::vector<char>
Patcher::loadFromArchive(const std::filesystem::path& archiveFile,
                         const std::filesystem::path& fileToExtract) noexcept(false) {
  Timer t(__FUNCTION__);
  spdlog::trace("loading from archive: {}", archiveFile.string());
  if (!filesystem::exists(archiveFile)) {
    throw runtime_error("File " + archiveFile.string() + " not found");
  }

  // Open the archive
  int err;
  zip* z = zip_open(archiveFile.c_str(), ZIP_RDONLY, &err);
  if (z == nullptr) {
    zip_error_t error;
    zip_error_init_with_code(&error, err);

    const string errorString = "error opening archive: "s + zip_error_strerror(&error);

    zip_error_fini(&error);

    throw runtime_error(errorString);
  }

  // Open the compressed file
  zip_file* f = zip_fopen(z, fileToExtract.c_str(), 0);
  if (f == nullptr) {
    const auto e = zip_get_error(z);
    throw runtime_error("zip_fopen() failed, "s + zip_error_strerror(e));
  }

  // Read the compressed file
  vector<char> result(bufferSize);
  zip_int64_t bytesRead = zip_fread(f, result.data(), bufferSize);
  if (bytesRead == -1) {
    const auto e = zip_get_error(z);
    throw runtime_error("zip_fread() failed, "s + zip_error_strerror(e));
  }

  zip_fclose(f);
  zip_close(z);

  result.resize(bytesRead);

  spdlog::trace("success");
  return result;
}

std::vector<char> Patcher::loadFromFile(const std::filesystem::path& file) noexcept(false) {
  Timer t(__FUNCTION__);
  spdlog::debug("loading from file: {}", file.string());

  if (!filesystem::exists(file)) {
    throw runtime_error("File " + file.string() + " not found");
  }

  ifstream in(file);
  in.exceptions(ios::failbit | ios::badbit);
  spdlog::trace("file opened");

  size_t size = filesystem::file_size(file);
  vector<char> data(size);

  in.read(data.data(), size);

  spdlog::trace("read {} bytes", size);
  return data;
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

void Patcher::patchFile(const std::filesystem::path& archiveFile,
                        const std::filesystem::path& fileToExtract) const noexcept(false) {
  vector<char> data = loadFromArchive(archiveFile, fileToExtract);
  patch(data);

  fs::path targetFile = m_outputPath / fileToExtract;

  saveToFile(data, targetFile);
}

void Patcher::patchFile(const std::filesystem::path& file) const noexcept(false) {
  vector<char> data = loadFromFile(file);
  patch(data);

  fs::path targetFile = m_outputPath / file;

  saveToFile(data, targetFile);
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

std::string Patcher::readFileToString(const std::filesystem::path& file) noexcept(false) {
  vector<char> data = loadFromFile(file);
  return {data.begin(), data.end()};
}

void Patcher::saveToFile(const std::vector<char>& data,
                         const std::filesystem::path& file) noexcept(false) {
  Timer t(__FUNCTION__);
  spdlog::trace("saving to file: {}", file.string());

  fs::create_directories(file.parent_path());
  ofstream out(file);
  out.exceptions(ios::failbit | ios::badbit);
  out.write(data.data(), data.size());
}

std::filesystem::path Patcher::getGamePath() noexcept(false) {
  Timer t(__FUNCTION__);
  fs::path steamPath = getSteamPath();

  ifstream libraryFoldersFile(steamPath / "steamapps/libraryfolders.vdf");

  auto root = tyti::vdf::read(libraryFoldersFile);

  // iterate over libraries
  for (const auto& library : root.childs | views::values) {
    spdlog::trace("checking library {}", library->attribs["path"]);
    // skip empty libraries
    if (library->childs["apps"] == nullptr || library->childs["apps"]->attribs.empty()) {
      continue;
    }

    // iterate over keys in apps
    for (const auto& appID : library->childs["apps"]->attribs | views::keys) {
      if (appID == APPID) {
        fs::path gamePath =
            fs::path(library->attribs["path"]) / "steamapps/common/Call to Arms - Gates of Hell";
        spdlog::trace("found game in {}", gamePath.string());
        return gamePath;
      }
    }
  }

  throw runtime_error("failed to find game path");
}

std::filesystem::path Patcher::getSteamPath() noexcept(false) {
  Timer t(__FUNCTION__);

  fs::path home                = getenv("HOME");
  static constexpr array paths = {".local/share/Steam", ".steam/steam",
                                  ".var/app/com.valvesoftware.Steam/.local/share/Steam"};

  for (const auto& path : paths) {
    fs::path p = home / path;
    if (fs::exists(p)) {
      spdlog::trace("found steam path: {}", p.string());
      return p;
    }
  }
  throw runtime_error("Could not find Steam installation");
}

Patcher::data_t Patcher::extractNumberFromString(const std::string& line) noexcept(false) {
  size_t firstDigit = line.find_first_of("0123456789");
  if (firstDigit == string::npos) {
    throw runtime_error("Failed to find digit");
  }

  size_t numberLength = 0;
  while (isdigit(line[firstDigit + numberLength])) {
    numberLength++;
  }

  string numberString = line.substr(firstDigit, numberLength);

  spdlog::trace("found number: {}", numberString);

  return {firstDigit, numberLength, stoi(numberString)};
}

void Patcher::multiplyNumberInString(std::string& line, int multiplier) noexcept(false) {
  spdlog::trace("multiplying number in string '{}' with {}", line, multiplier);

  auto [offset, size, number] = extractNumberFromString(line);
  line.replace(offset, size, to_string(number * multiplier));

  spdlog::trace("replaced number {} with {}", number, number * multiplier);
}

void Patcher::replaceNumberInString(std::string& line, int newValue) noexcept(false) {
  spdlog::trace("replacing number in string '{}' with {}", line, newValue);

  auto [offset, size, number] = extractNumberFromString(line);
  line.replace(offset, size, to_string(newValue));

  spdlog::trace("replaced number {} with {}", number, newValue);
}

sha256sum Patcher::sha256(const std::filesystem::path& file) noexcept(false) {
  Timer t(__FUNCTION__ + " "s + file.string());

  // check if file exists
  if (!fs::exists(file)) {
    throw runtime_error("file does not exist");
  }

  ifstream input(file, ios::binary);

  MdCtxPtr mdctx(EVP_MD_CTX_new());
  unsigned int digestLength;

  if (mdctx == nullptr) {
    throw runtime_error("EVP_MD_CTX_new error");
  }

  // initialize
  if (1 != EVP_DigestInit_ex(mdctx.get(), EVP_sha256(), nullptr)) {
    throw runtime_error("EVP_DigestInit_ex error");
  }

  constexpr size_t buffer_size{1 << 12};
  vector buffer(buffer_size, '\0');

  while (input.good()) {
    input.read(buffer.data(), buffer_size);
    if (1 != EVP_DigestUpdate(mdctx.get(), buffer.data(), input.gcount())) {
      throw runtime_error("EVP_DigestUpdate error");
    }
  }

  // allocate memory
  DigestPtr digest(static_cast<unsigned char*>(OPENSSL_malloc(EVP_MD_size(EVP_sha256()))));

  if (digest == nullptr) {
    throw runtime_error("OPENSSL_malloc error");
  }

  // finalize data
  if (1 != EVP_DigestFinal_ex(mdctx.get(), digest.get(), &digestLength)) {
    throw runtime_error("EVP_DigestFinal_ex error");
  }

  sha256sum checksum;
  memcpy(checksum.data(), digest.get(), digestLength);

  return checksum;
}

void Patcher::ltrim(std::string& line) noexcept {
  line.erase(line.begin(), ranges::find_if(line, [](char c) {
               return !isspace(c);
             }));
}

void Patcher::rtrim(std::string& line) noexcept {
  line.erase(find_if(line.rbegin(), line.rend(),
                     [](char c) {
                       return !isspace(c);
                     })
                 .base(),
             line.end());
}

void Patcher::trim(std::string& line) noexcept {
  ltrim(line);
  rtrim(line);
}
