#include <filesystem>
#include <iostream>
#include <regex>
#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "Patcher.h"
#include "Mods.h"

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

  argparse::ArgumentParser program("resupply_patcher", "1.0", argparse::default_arguments::help);

  program.add_argument("-v", "--verbose")
         .action([&](const auto&) {
           verbosity++;
         })
         .append()
         .flag()
         .nargs(0)
         .help("increase output verbosity")
         .flag();

  auto &modGroup = program.add_mutually_exclusive_group();
  modGroup.add_argument("-V", "--valour").help("patch valour").flag();
  modGroup.add_argument("-H", "--hotmod").help("patch hotmod").flag();
  modGroup.add_argument("-W", "--west81").help("patch west81").flag();
  modGroup.add_argument("-M", "--mace").help("patch mace").flag();

  program.add_argument("out").help("output directory").required();

  try {
    program.parse_args(argc, argv);
  } catch (const exception& err) {
    cerr << err.what() << "\n";
    cerr << program;
    return 1;
  }

  spdlog::set_level(verbosityToLogLevel(verbosity));

  fs::path outDir = program.get<string>("out");
  Patcher p(outDir);

  try {
    if (program.is_used("--valour")) {
      p.patchMod(mods::valour);
      p.removeValourResupplyRestrictions();
    } else if (program.is_used("--hotmod")) {
      p.patchVanilla(); // hotmod 1968 does not overwrite the original "resupply.inc"
      p.patchMod(mods::hotmod);
    } else if (program.is_used("--west81")) {
      // hotmod 1968 is a dependency of west 81, so it has to be patched as well
      p.patchVanilla();
      p.patchMod(mods::hotmod);
      p.patchMod(mods::west81);
    } else if (program.is_used("--mace"))  {
      p.patchMod(mods::mace);
    } else {
      p.patchVanilla();
    }
  } catch (const runtime_error& ex) {
    cerr << "Error while patching: " << ex.what() << "\n";
  }

  return 0;
}
