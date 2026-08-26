#include <argparse/argparse.hpp>
#include <exception>
#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

#include "Mods.h"
#include "Patcher.h"
#include "version.h"

using namespace std;
namespace fs = std::filesystem;

spdlog::level::level_enum verbosityToLogLevel(int verbosity) {
  switch (verbosity) {
  case 1:
    return spdlog::level::info;
  case 2:
    return spdlog::level::debug;
  case 3:
    return spdlog::level::trace;
  default:
    return spdlog::level::err;
  }
}

int main(int argc, char** argv) {
  int verbosity = 0;

  argparse::ArgumentParser program("resupply_patcher", RESUPPLY_PATCHER_VERSION);

  program.add_argument("-V", "--verbose")
      .action([&](const auto&) {
        verbosity++;
      })
      .append()
      .flag()
      .nargs(0)
      .help("increase output verbosity");

  program.add_argument("out").help("output directory").required();

  auto& modGroup = program.add_mutually_exclusive_group();
  modGroup.add_argument("--valour").help("patch valour").flag();
  modGroup.add_argument("--hotmod").help("patch hotmod 1986").flag();
  modGroup.add_argument("--west81").help("patch west81").flag();
  modGroup.add_argument("--mace").help("patch mace").flag();
  modGroup.add_argument("--hortens-frontline").help("patch hortens frontline").flag();
  modGroup.add_argument("--indomitus").help("patch indomitus").flag();

  auto& inputPathsGroup = program.add_group("input paths");
  inputPathsGroup.add_argument("--library", "-l").help("steam library path");
  inputPathsGroup.add_argument("--workshop", "-w").help("steam workshop path");
  inputPathsGroup.add_argument("--game", "-g").help("game path");

  try {
    program.parse_args(argc, argv);
  } catch (const exception& err) {
    cerr << err.what() << "\n";
    cerr << program;
    return 1;
  }

  spdlog::set_level(verbosityToLogLevel(verbosity));

  fs::path outDir = program.get<string>("out");

  bool autodetect = true;
  if (program.is_used("--library") || program.is_used("--workshop") || program.is_used("--game")) {
    autodetect = false;
  }

  Patcher p(outDir, autodetect);

  if (!autodetect) {
    if (auto value = program.present("--library")) {
      p.setLibraryPath(*value);
    }
    if (auto value = program.present("--workshop")) {
      p.setWorkshopPath(*value);
    }
    if (auto value = program.present("--game")) {
      p.setGamePath(*value);
    }
  }

  try {
    if (program.is_used("--valour")) {
      p.patchMod(mods::Valour);
      p.removeResupplyRestrictions(mods::Valour);
    } else if (program.is_used("--hotmod")) {
      p.patchVanilla();  // hotmod 1968 does not overwrite the original "resupply.inc"
      p.patchMod(mods::Hotmod);
    } else if (program.is_used("--west81")) {
      // todo: check if `resupply.inc` even gets loaded as mod contains `resuppply_vanilla.inc`
      p.patchVanilla();  // west 81 does not overwrite the original "resupply.inc"
      p.patchMod(mods::West81);
    } else if (program.is_used("--mace")) {
      p.patchMod(mods::Mace);
    } else if (program.is_used("--hortens-frontline")) {
      p.patchMod(mods::HortensFrontline);
    } else if (program.is_used("--indomitus")) {
      p.patchMod(mods::Indomitus);
    } else {
      p.patchVanilla();
    }
  } catch (const runtime_error& ex) {
    cerr << "Error while patching: " << ex.what() << "\n";
  }

  return 0;
}
