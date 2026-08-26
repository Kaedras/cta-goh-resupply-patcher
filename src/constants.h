#pragma once

#include <regex>

constexpr size_t bufferSize  = 1024 * 1024;
constexpr auto appidString   = "400750";
constexpr auto gameDirectory = "steamapps/common/Call to Arms - Gates of Hell";

namespace re {
// patterns for changing resupply values
const std::regex radius(R"(\{radius\s*\d+)");
const std::regex resupplyPeriod(R"(\{resupplyPeriod\s*\d+)");
const std::regex regenerationPeriod(R"(\{regenerationPeriod\s*\d+)");
const std::regex limit(R"(\{limit\s*\d+)");
const std::regex limitSpecial(R"(\{limit\s*%supply)");

const std::regex resupply(R"(\{resupply\r\n([\s\S]+?)\t+\}\r\n)");

// patterns to use when replacing lines
const std::regex itemsLight(R"(\(define "items_light_\w+\"\r\n([\s\S]+?)\r\n\))");
const std::regex itemsHeavy(R"(\(define "items_heavy_\w+\"\r\n([\s\S]+?)\r\n\))");
const std::regex itemsMedic(R"(\(define "items_medic\w*\"\r\n([\s\S]+?)\r\n\))");
const std::regex itemsEngineer(R"(\(define "items_engineer"\r\n([\s\S]+?)\r\n\))");
const std::regex itemsExplosives(R"(\(define "items_explosives"\r\n([\s\S]+?)\r\n\))");
const std::regex resupplyItemsLight(R"(\("items_light(?!_all)\w{1,8}"\))");
const std::regex resupplyItemsHeavy(R"(\("items_heavy(?!_all)\w{1,8}"\))");
const std::regex resupplyItemsMedic(R"(\("items_medic(?!_all)\w{0,5}"\))");

// patterns to use when removing lines to prevent excessive amount of empty lines
const std::regex itemsLightRemove(R"(\w*\(define "items_light_\w+\"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
const std::regex itemsHeavyRemove(R"(\w*\(define "items_heavy_\w+\"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
const std::regex itemsMedicRemove(R"(\w*\(define "items_medic\w*\"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
const std::regex
    itemsEngineerRemove(R"(\w*\(define "items_engineer"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");
const std::regex
    itemsExplosivesRemove(R"(\w*\(define "items_explosives"\r\n([\s\S]+?)\r\n\)(?:\r\n)+)");

const std::regex steamLibrary(R"-(\s+"path"\s+"(.*)")-");
}  // namespace re
