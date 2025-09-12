#pragma once

#include <chrono>
#include <string>

class Timer {
public:
  Timer(std::string description);
  ~Timer();

private:
  std::string                                    m_description;
  std::chrono::high_resolution_clock::time_point m_start;
};
