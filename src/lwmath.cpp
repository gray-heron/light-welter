
#include "lwmath.h"

float lerp(float t, float v1, float v2) { return (1.0f - t) * v1 + t * v2; }