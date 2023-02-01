#pragma once
#include <cstdlib>

inline float RandomFloat(float min, float max) { return rand() * 1.0 / RAND_MAX * (max - min + 1) + min; }
