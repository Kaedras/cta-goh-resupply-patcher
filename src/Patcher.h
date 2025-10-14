#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include "mods/Mod.h"

using sha256sum = std::array<char, 32>;

class Patcher {
public:
  /**
   * @throw std::runtime_error When the game directory cannot be found
   */
  explicit Patcher(std::filesystem::path outputDir) noexcept(false);

  ~Patcher() noexcept;

  /**
   * @brief Patch vanilla resupply values
   * @throw std::runtime_error
   */
  void patchVanilla() const noexcept(false);

  void patchMod(const Mod& mod) const noexcept(false);

  /**
   * @brief Remove resupply restrictions for a mod
   * @note This function has not been tested for mods other than Valour
   */
  void removeResupplyRestrictions(const Mod& mod) const;

private:
  static constexpr size_t bufferSize = 1024 * 1024;

  /**
   * @brief Extract a file from an archive
   * @param archiveFile Archive file to read
   * @param fileToExtract File to extract from inside the archive
   * @throw std::runtime_error
   */
  static std::vector<char> loadFromArchive(const std::filesystem::path& archiveFile, const std::filesystem::path& fileToExtract) noexcept(false);

  /**
   * @brief Read the specified file
   * @param file File to read
   * @throw std::runtime_error
   */
  static std::vector<char> loadFromFile(const std::filesystem::path& file) noexcept(false);

  /**
   * @brief Read a text file and store its contents in a string
   * @param file File to read
   * @return String containing the file contents
   * @throw std::runtime_error
   */
  static std::string readFileToString(const std::filesystem::path& file) noexcept(false);

  /**
   * @brief Patch resupply values of the provided data
   * @param data File data to patch
   * @throw std::runtime_error
   */
  static void patch(std::vector<char>& data) noexcept(false);

  /**
   * @brief Save the provided data to the specified path
   * @param data Data to save
   * @param file Output file
   * @throw std::runtime_error
   */
  static void saveToFile(const std::vector<char>& data, const std::filesystem::path& file) noexcept(false);

  /**
   * @brief Get the game path
   * @throw std::runtime_error
   */
  static std::filesystem::path getGamePath() noexcept(false);

  /**
   * @brief Get the steam directory
   * @throw std::runtime_error
   */
  static std::filesystem::path getSteamPath() noexcept(false);

  /**
   * @brief Extract a file from an archive, patch the resupply values, and save it in @link m_outputPath @endlink
   * @param archiveFile Archive to extract from
   * @param fileToExtract File to extract from inside the archive
   * @throw std::runtime_error
   */
  void patchFile(const std::filesystem::path& archiveFile, const std::filesystem::path& fileToExtract) const noexcept(false);

  /**
   * @brief Extract item lists from all files in the provided path and replace them with includes
   */
  void generateItemsAll(const Mod& mod) const;

  void replaceResupply(const Mod& mod) const;

  /**
   * @brief Data structure representing a number inside a string.
   */
  struct data_t {
    size_t offset;
    size_t size;
    int value;
  };

  /**
   * @brief Extracts the first number found in the given string.
   * @param line The string to search for a number.
   * @throw std::runtime_error
   */
  static data_t extractNumberFromString(const std::string& line) noexcept(false);

  /**
   * @brief Multiplies the first number inside a string with the given multiplier.
   * @param line The string to search for a number.
   * @param multiplier Multiplier to use.
   * @throw std::runtime_error
   */
  static void multiplyNumberInString(std::string& line, int multiplier) noexcept(false);

  /**
   * @brief Replaces the first number inside a string with the given value.
   * @param line The string to search for a number.
   * @param newValue New value to use.
   * @throw std::runtime_error
   */
  static void replaceNumberInString(std::string& line, int newValue) noexcept(false);

  static sha256sum sha256(const std::filesystem::path& file) noexcept(false);

  static void ltrim(std::string& line) noexcept;
  static void rtrim(std::string& line) noexcept;
  static void trim(std::string& line) noexcept;

  std::filesystem::path m_outputPath;
  std::filesystem::path m_gamePath;
  std::filesystem::path m_workshopPath;

  std::unordered_map<std::filesystem::path, sha256sum> m_outputChecksums;
};
