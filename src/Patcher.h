#pragma once

#include "types.h"

#include <array>
#include <filesystem>
#include <unordered_map>
#include <vector>

class Mod;

class Patcher {
public:
  /**
   * @param outputDir The output directory
   * @param autodetect Enable or disable autodetection of steam library directory
   * @throw std::runtime_error
   */
  explicit Patcher(std::filesystem::path outputDir, bool autodetect = true) noexcept(false);

  ~Patcher() noexcept;

  /**
   * @brief Attempt to set the game and workshop paths based on the provided library path
   * @param libraryPath Path to the steam library
   */
  void setLibraryPath(const std::filesystem::path& libraryPath) noexcept;

  /**
   * @brief Set the game path
   * @param gamePath Path to the game
   */
  void setGamePath(const std::filesystem::path& gamePath) noexcept;

  /**
   * @brief Set the steam workshop path
   * @param workshopPath Path to the steam workshop directory
   */
  void setWorkshopPath(const std::filesystem::path& workshopPath) noexcept;

  /**
   * @brief Patch vanilla resupply values
   * @throw std::runtime_error
   */
  void patchVanilla() const noexcept(false);

  /**
   * @brief Patch resupply values a mod
   * @param mod Mod to patch
   * @throw std::runtime_error
   */
  void patchMod(const Mod& mod) const noexcept(false);

  /**
   * @brief Remove resupply restrictions for a mod
   * @note This function has not been tested for mods other than Valour
   */
  void removeResupplyRestrictions(const Mod& mod) const;

private:
  /**
   * @brief Patch resupply values of the provided data
   * @param data File data to patch
   * @throw std::runtime_error
   */
  static void patch(std::vector<char>& data) noexcept(false);

  /**
   * @brief Extract a file from an archive, patch the resupply values, and save it in
   * @link m_outputPath @endlink
   * @param archiveFile Archive to extract from
   * @param fileToExtract File to extract from inside the archive
   * @throw std::runtime_error
   */
  void patchFileFromArchive(const std::filesystem::path& archiveFile,
                            const std::filesystem::path& fileToExtract) const noexcept(false);

  /**
   * @brief Patch the resupply values of a file and save it in the provided path
   * @param inputFile File to patch
   * @param outputFile Output file path
   * @throw std::runtime_error
   */
  static void patchFile(const std::filesystem::path& inputFile,
                        const std::filesystem::path& outputFile) noexcept(false);

  /**
   * @brief Extract item lists from all files in the provided path and replace them with includes
   */
  void generateItemsAll(const Mod& mod) const;

  void replaceResupply(const Mod& mod) const;

  std::filesystem::path m_outputPath;
  std::filesystem::path m_gamePath;
  std::filesystem::path m_workshopPath;

  std::unordered_map<std::filesystem::path, sha256sum> m_outputChecksums;
};
