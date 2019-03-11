
#pragma once
#include <glm/glm.hpp>

#include "scene.h"

class Raytracer
{
  public:
    boost::optional<Intersection> Trace(glm::vec3 source, glm::vec3 target,
                                        const Scene &scene);
};