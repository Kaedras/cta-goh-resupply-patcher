cmake_minimum_required(VERSION 3.31)
project(resupply_patcher)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
# fetch argparse
FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
        GIT_TAG v3.2
)
# fetch spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
)
# fetch libzip
FetchContent_Declare(
        libzip
        GIT_REPOSITORY https://github.com/nih-at/libzip.git
        GIT_TAG v1.11.3
        FIND_PACKAGE_ARGS
)
# disable unneeded libzip options
set(ENABLE_COMMONCRYPTO OFF)
set(ENABLE_GNUTLS OFF)
set(ENABLE_MBEDTLS OFF)
set(ENABLE_OPENSSL OFF)
set(ENABLE_WINDOWS_CRYPTO OFF)
set(ENABLE_BZIP2 OFF)
set(ENABLE_LZMA OFF)
set(ENABLE_ZSTD OFF)
set(BUILD_TOOLS OFF)
set(BUILD_REGRESS OFF)
set(BUILD_OSSFUZZ OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_DOC OFF)

FetchContent_MakeAvailable(argparse spdlog libzip)

# fetch vdf parser
file(DOWNLOAD
        https://raw.githubusercontent.com/TinyTinni/ValveFileVDF/e15aa4cfe52761c71ce8f5c27b5f50c523c25e58/include/vdf_parser.hpp
        deps/vdf_parser.hpp
        EXPECTED_HASH SHA512=f2d85be10941c0b5da74c10a19b1f74f068188a246b2cb34f5af31c84d45f29bb0a22a0ffe36fa9c8310a101d1be6a94a263882c7534c93e483e42bef802d657
)

find_package(OpenSSL CONFIG REQUIRED)

add_executable(resupply_patcher
        src/main.cpp
        src/Item.h
        src/Patcher.cpp
        src/Patcher.h
        src/Timer.cpp
        src/Timer.h
        src/Mods.h
        src/mods/Valour.h
        src/mods/Hotmod1986.h
        src/mods/West81.h
        src/mods/Mace.h
)

target_compile_options(resupply_patcher PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(resupply_patcher argparse spdlog::spdlog zip OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(resupply_patcher PRIVATE ${CMAKE_BINARY_DIR}/deps)
