#pragma once

#include <array>
#include <memory>
#include <openssl/evp.h>
#include <openssl/types.h>

using sha256sum = std::array<char, 32>;

/**
 * @brief Data structure representing a number inside a string.
 */
struct data_t {
  std::size_t offset;
  std::size_t size;
  int value;
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
