#ifndef MPM_METHOD_SNOWFALL_H
#define MPM_METHOD_SNOWFALL_H
#include <glm/vec3.hpp>

namespace Snowfall
{
    inline constexpr glm::vec3 boxLeftCorner(1.5f, 4.0f, 1.5f);
    inline constexpr glm::vec3 boxRightCorner(3.6f, 4.02f, 3.6f);
    inline constexpr int particleCount = 100;
    inline constexpr float snowDensity = 100.0f;
}

#endif
