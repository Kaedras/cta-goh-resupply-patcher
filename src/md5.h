#pragma once

#include <array>        // for array
#include <cstdint>      // for uint8_t, uint32_t, uint64_t
#include <cstdio>       // for FILE, size_t
#include <string_view>  // for string_view

// modified version of https://github.com/Zunawe/md5-c

struct MD5Context {
  uint64_t size;                   // Size of input in bytes
  std::array<uint32_t, 4> buffer;  // Current accumulation of hash
  std::array<uint8_t, 64> input;   // Input to be used in the next step
  std::array<uint8_t, 16> digest;  // Result of algorithm
  MD5Context();
};

void md5Init(MD5Context* ctx);
void md5Update(MD5Context* ctx, const uint8_t* input_buffer, size_t input_len);
void md5Finalize(MD5Context* ctx);
void md5Step(std::array<uint32_t, 4>& buffer, const std::array<uint32_t, 16>& input);

std::array<uint8_t, 16> md5String(std::string_view input);
std::array<uint8_t, 16> md5File(FILE* file);
