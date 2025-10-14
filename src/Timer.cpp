#include "Timer.h"

#include "spdlog/fmt/bundled/base.h"
#include "spdlog/fmt/bundled/format.h"
#include <spdlog/spdlog.h>
#include <utility>

using namespace std;

Timer::Timer(std::string description) : m_description(std::move(description)) {
  m_start = chrono::high_resolution_clock::now();
}

Timer::~Timer() {
  const auto end = chrono::high_resolution_clock::now();
  spdlog::debug("{}: {} µs", m_description, (end - m_start) / 1us);
}
