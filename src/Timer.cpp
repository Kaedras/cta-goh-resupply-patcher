#include "Timer.h"

#include <spdlog/spdlog.h>

using namespace std;

Timer::Timer(std::string description) : m_description(move(description)) {
  m_start = std::chrono::high_resolution_clock::now();
}

Timer::~Timer() {
  const auto end = std::chrono::high_resolution_clock::now();
  spdlog::debug("{}: {} µs", m_description, (end - m_start) / 1us);
}
