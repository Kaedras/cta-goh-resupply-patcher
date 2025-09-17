#pragma once

struct Settings {
  struct defaults {
    static constexpr int resupplyPeriod = 1;
    static constexpr int regenerationPeriod = 1;
    static constexpr int radiusMultiplier = 4;
    static constexpr int limitMultiplier = 10;
    // value to use in case the limit is "%supply" instead of an integer
    static constexpr int limitFallback = 2500;
  };

  int resupplyPeriod = defaults::resupplyPeriod;
  int regenerationPeriod = defaults::regenerationPeriod;

  int radiusMultiplier = defaults::radiusMultiplier;
  int limitMultiplier = defaults::limitMultiplier;
  int limitFallback = defaults::limitFallback;
};
