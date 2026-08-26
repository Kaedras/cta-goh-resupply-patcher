#include "utils.h"

#include "Timer.h"
#include "constants.h"

#include <cstring>
#include <openssl/evp.h>
#include <spdlog/spdlog.h>
#include <vdf_parser.hpp>
#include <zip.h>

using namespace std;
namespace fs = std::filesystem;

std::generator<const std::filesystem::path&> getSteamLibraries() {
  fs::path steamPath = getSteamPath();

  ifstream libraryFoldersFile(steamPath / "steamapps/libraryfolders.vdf");

  auto root = tyti::vdf::read(libraryFoldersFile);

  for (const auto& library : root.childs | views::values) {
    // skip empty libraries
    if (library->childs["apps"] == nullptr) {
      spdlog::trace("skipping empty library {}", library->attribs["path"]);
      continue;
    }

    co_yield library->attribs["path"];
  }
}

fs::path getWorkshopPath(const fs::path& libraryPath) noexcept(false) {
  return fs::canonical(libraryPath / "workshop/content/400750");
}

std::filesystem::path getGamePath(const std::filesystem::path& libraryPath) noexcept(false) {
  Timer t(__FUNCTION__);
  if (libraryPath.empty()) {
    for (const auto& library : getSteamLibraries()) {
      spdlog::trace("checking library {}", library.string());

      fs::path gamePath = library / gameDirectory;
      if (fs::exists(gamePath)) {
        return gamePath;
      }
    }
  } else {
    fs::path gamePath = libraryPath / gameDirectory;
    spdlog::trace("found game in {}", gamePath.string());
    if (fs::exists(gamePath)) {
      return gamePath;
    }
  }

  throw runtime_error("failed to find game path");
}

std::vector<char> loadFromArchive(const std::filesystem::path& archiveFile,
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

std::vector<char> loadFromFile(const std::filesystem::path& file) noexcept(false) {
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

std::string readFileToString(const std::filesystem::path& file) noexcept(false) {
  vector<char> data = loadFromFile(file);
  return {data.begin(), data.end()};
}

void saveToFile(const std::vector<char>& data, const std::filesystem::path& file) noexcept(false) {
  Timer t(__FUNCTION__);
  spdlog::trace("saving to file: {}", file.string());

  fs::create_directories(file.parent_path());
  ofstream out(file);
  out.exceptions(ios::failbit | ios::badbit);
  out.write(data.data(), data.size());
}

std::filesystem::path getSteamPath() noexcept(false) {
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

data_t extractNumberFromString(const std::string& line) noexcept(false) {
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

void multiplyNumberInString(std::string& line, int multiplier) noexcept(false) {
  spdlog::trace("multiplying number in string '{}' with {}", line, multiplier);

  auto [offset, size, number] = extractNumberFromString(line);
  line.replace(offset, size, to_string(number * multiplier));

  spdlog::trace("replaced number {} with {}", number, number * multiplier);
}

void replaceNumberInString(std::string& line, int newValue) noexcept(false) {
  spdlog::trace("replacing number in string '{}' with {}", line, newValue);

  auto [offset, size, number] = extractNumberFromString(line);
  line.replace(offset, size, to_string(newValue));

  spdlog::trace("replaced number {} with {}", number, newValue);
}

sha256sum sha256(const std::filesystem::path& file) noexcept(false) {
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

void ltrim(std::string& line) noexcept {
  line.erase(line.begin(), ranges::find_if(line, [](char c) {
               return !isspace(c);
             }));
}

void rtrim(std::string& line) noexcept {
  line.erase(find_if(line.rbegin(), line.rend(),
                     [](char c) {
                       return !isspace(c);
                     })
                 .base(),
             line.end());
}

void trim(std::string& line) noexcept {
  ltrim(line);
  rtrim(line);
}
