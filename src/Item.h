#pragma once

#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Helper class to parse items
 */
class Item {
public:
  std::vector<std::string> strings;
  int unknown = -1;
  int value = -1;
  std::string condition; // condition before item definition, e.g. '(mod not "mp"'

  friend std::ostream& operator<<(std::ostream& out, const Item& i) {
    if (!i.condition.empty()) {
      out << "\t" << i.condition << "\r\n\t";
    }
    out << "\t{item";
    for (const auto& str : i.strings) {
      out << " \"" << str << "\"";
    }
    out << " " << i.unknown << " {value " << i.value << "}}";
    if (!i.condition.empty()) {
      out << "\r\n\t)";
    }
    return out;
  }

  friend std::istream& operator>>(std::istream& in, Item& i) {
    std::string line;
    getline(in, line);

    std::istringstream iss(line);
    std::string token;

    // Skip "{item"
    iss >> token;

    // Read strings until we hit a number
    while (iss >> token) {
      if (isdigit(token[0])) {
        i.unknown = std::stoi(token);
        break;
      }
      // Remove quotes
      token = token.substr(1, token.length() - 2);
      i.strings.push_back(token);
    }

    // Skip "{value"
    iss >> token;
    // Read value
    iss >> i.value;

    return in;
  }
};
