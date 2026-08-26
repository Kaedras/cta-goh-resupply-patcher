#pragma once

#include "types.h"

#include <filesystem>
#include <generator>
#include <vector>

std::generator<const std::filesystem::path&> getSteamLibraries();

std::filesystem::path getWorkshopPath(const std::filesystem::path& libraryPath) noexcept(false);

/**
 * @brief Get the game path
 * @throw std::runtime_error
 */
std::filesystem::path getGamePath(const std::filesystem::path& libraryPath = {}) noexcept(false);

/**
 * @brief Extract a file from an archive
 * @param archiveFile Archive file to read
 * @param fileToExtract File to extract from inside the archive
 * @throw std::runtime_error
 */
std::vector<char> loadFromArchive(const std::filesystem::path& archiveFile,
                                  const std::filesystem::path& fileToExtract) noexcept(false);

/**
 * @brief Read the specified file
 * @param file File to read
 * @throw std::runtime_error
 */
std::vector<char> loadFromFile(const std::filesystem::path& file) noexcept(false);

/**
 * @brief Read a text file and store its contents in a string
 * @param file File to read
 * @return String containing the file contents
 * @throw std::runtime_error
 */
std::string readFileToString(const std::filesystem::path& file) noexcept(false);

/**
 * @brief Save the provided data to the specified path
 * @param data Data to save
 * @param file Output file
 * @throw std::runtime_error
 */
void saveToFile(const std::vector<char>& data, const std::filesystem::path& file) noexcept(false);

/**
 * @brief Get the steam directory
 * @throw std::runtime_error
 */
std::filesystem::path getSteamPath() noexcept(false);

/**
 * @brief Extracts the first number found in the given string.
 * @param line The string to search for a number.
 * @throw std::runtime_error
 */
data_t extractNumberFromString(const std::string& line) noexcept(false);

/**
 * @brief Multiplies the first number inside a string with the given multiplier.
 * @param line The string to search for a number.
 * @param multiplier Multiplier to use.
 * @throw std::runtime_error
 */
void multiplyNumberInString(std::string& line, int multiplier) noexcept(false);

/**
 * @brief Replaces the first number inside a string with the given value.
 * @param line The string to search for a number.
 * @param newValue New value to use.
 * @throw std::runtime_error
 */
void replaceNumberInString(std::string& line, int newValue) noexcept(false);

/**
 * @brief Calculate the SHA256 sum of a file
 */
sha256sum sha256(const std::filesystem::path& file) noexcept(false);

void ltrim(std::string& line) noexcept;
void rtrim(std::string& line) noexcept;
void trim(std::string& line) noexcept;
