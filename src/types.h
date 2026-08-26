#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>

using md5sum = std::array<uint8_t, 16>;

/**
 * @brief Data structure representing a number inside a string.
 */
struct data_t {
  std::size_t offset;
  std::size_t size;
  int value;
};

struct FileCloser {
  void operator()(FILE* f) const {
    if (f) {
      fclose(f);
    }
  }
};
using FilePtr = std::unique_ptr<FILE, FileCloser>;
