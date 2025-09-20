#include "Patcher.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>
#include <vdf_parser.hpp>
#include <zip.h>
#include "Item.h"
#include "Mods.h"
#include "Settings.h"
#include "Timer.h"

using namespace std;
namespace fs = std::filesystem;

namespace {
  constexpr auto APPID = "400750";

  const regex radius(R"(\{radius\s*\d+)");
  const regex resupplyPeriod(R"(\{resupplyPeriod\s*\d+)");
  const regex regenerationPeriod(R"(\{regenerationPeriod\s*\d+)");
  const regex limit(R"(\{limit\s*\d+)");
  const regex limitSpecial(R"(\{limit\s*%supply)");

} // namespace

Patcher::Patcher(std::filesystem::path outputDir) noexcept(false) : m_outputPath(std::move(outputDir)), m_gamePath(getGamePath()), m_workshopPath(m_gamePath / "../../workshop/content/400750") {}

void Patcher::patchVanilla() const noexcept(false) {
  patchFile(m_gamePath / "resource/properties.pak", "properties/resupply.inc");
}

std::vector<char> Patcher::loadFromArchive(const std::filesystem::path& archiveFile, const std::filesystem::path& fileToExtract) noexcept(false) {
  Timer t(__FUNCTION__);
  spdlog::trace("loading from archive: {}", archiveFile.string());
  if (!filesystem::exists(archiveFile)) {
    throw runtime_error("File not found");
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
    throw runtime_error("File not found");
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
    // but as this function only takes a few milliseconds, the practical benefit of any optimization is negligible
    if (regex_search(line, regex(radius))) {
      // radius
      spdlog::trace("modifying radius");
      multiplyNumberInString(line, Settings::defaults::radiusMultiplier);
    } else if (regex_search(line, regex(resupplyPeriod))) {
      // resupply period
      spdlog::trace("modifying resupply period");
      replaceNumberInString(line, Settings::defaults::resupplyPeriod);
    } else if (regex_search(line, regex(regenerationPeriod))) {
      // regeneration period
      spdlog::trace("modifying regeneration period");
      replaceNumberInString(line, Settings::defaults::regenerationPeriod);
    } else if (regex_search(line, regex(limit))) {
      // limit
      spdlog::trace("modifying limit");
      multiplyNumberInString(line, Settings::defaults::limitMultiplier);
    } else if (regex_search(line, regex(limitSpecial))) {
      // limit, value is "%supply" instead of an integer
      spdlog::trace("modifying limit %supply");
      static constexpr string stringToReplace = "%supply";
      line.replace(line.find(stringToReplace), stringToReplace.size(), to_string(Settings::defaults::limitFallback));
    }

    // rtrim(line);
    line.append("\r\n");
    out.append_range(line);
  }

  swap(data, out);
}

void Patcher::patchFile(const std::filesystem::path& archiveFile, const std::filesystem::path& fileToExtract) const noexcept(false) {
  vector<char> data = loadFromArchive(archiveFile, fileToExtract);
  patch(data);

  fs::path targetFile = m_outputPath / fileToExtract;

  if (isExistingFileIdentical(data, targetFile)) {
    spdlog::trace("not saving to file {} because contents are identical", targetFile.string());
  } else {
    saveToFile(data, targetFile);
  }
}

void Patcher::saveToFile(const std::vector<char>& data, const std::filesystem::path& file) noexcept(false) {
  Timer t(__FUNCTION__);
  spdlog::trace("saving to file: {}", file.string());
  cout << "\033[33m" << "contents of " << file.string() << " have changed\033[0m\n";
  fs::create_directories(file.parent_path());
  ofstream out(file);
  out.exceptions(ios::failbit | ios::badbit);
  out.write(data.data(), data.size());
}

bool Patcher::isExistingFileIdentical(const std::vector<char>& data, const std::filesystem::path& file) noexcept {
  Timer t(__FUNCTION__);
  spdlog::trace("comparing data to existing file {}", file.string());

  // check if file exists
  if (!fs::exists(file)) {
    spdlog::trace("no existing file");
    return false;
  }

  // check if file sizes are identical
  size_t existingSize = fs::file_size(file);
  if (existingSize != data.size()) {
    spdlog::trace("file size does not match");
    return false;
  }

  try {
    // read file and compare contents
    ifstream in(file);
    in.exceptions(ios::failbit | ios::badbit);
    vector<char> existingData;
    existingData.reserve(fs::file_size(file));
    in.read(existingData.data(), existingSize);

    bool result = memcmp(data.data(), existingData.data(), data.size()) == 0;
    spdlog::trace("files are{}identical", result ? " not " : " ");
    return result;
  } catch (...) {
    spdlog::trace("exception while reading, assuming files are different");
    return false;
  }
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
        fs::path gamePath = fs::path(library->attribs["path"]) / "steamapps/common/Call to Arms - Gates of Hell";
        spdlog::trace("found game in {}", gamePath.string());
        return gamePath;
      }
    }
  }

  throw runtime_error("failed to find game path");
}

std::filesystem::path Patcher::getSteamPath() noexcept(false) {
  Timer t(__FUNCTION__);

  fs::path home = getenv("HOME");
  static constexpr array paths = {".local/share/Steam", ".steam/steam", ".var/app/com.valvesoftware.Steam/.local/share/Steam"};

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
