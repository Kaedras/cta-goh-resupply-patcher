cmake_minimum_required(VERSION 3.31)
project(resupply_patcher LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)

if(NOT VCPKG_TARGET_TRIPLET)
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
            GIT_TAG v1.17.0
    )
    # fetch libzip
    FetchContent_Declare(
            libzip
            GIT_REPOSITORY https://github.com/nih-at/libzip.git
            GIT_TAG v1.11.4
            FIND_PACKAGE_ARGS
    )

    # configure spdlog
    set(SPDLOG_USE_STD_FORMAT ON)

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
else()
    find_package(argparse CONFIG REQUIRED)
    find_package(libzip CONFIG REQUIRED)
    find_package(spdlog CONFIG REQUIRED)
endif()

find_package(OpenSSL REQUIRED)

add_library(patcher
    src/Item.h
    src/Patcher.cpp
    src/Patcher.h
    src/Timer.cpp
    src/Timer.h
    src/Mods.h
    src/mods/Valour.h
    src/mods/Hortens_Frontline.h
    src/mods/Hotmod1986.h
    src/mods/West81.h
    src/mods/Mace.h
    src/mods/Indomitus.h
    src/utils.cpp
    src/utils.h
    src/constants.h
    src/types.h
)
if(UNIX)
    target_compile_options(patcher PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(patcher PRIVATE libzip::zip OpenSSL::SSL OpenSSL::Crypto spdlog::spdlog)

add_executable(resupply_patcher
    src/main.cpp
)

configure_file(src/version.h.in version.h)
target_include_directories(resupply_patcher PUBLIC ${CMAKE_BINARY_DIR})

if(UNIX)
    target_compile_options(resupply_patcher PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(resupply_patcher PRIVATE argparse::argparse spdlog::spdlog patcher)
