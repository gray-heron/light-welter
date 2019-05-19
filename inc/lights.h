#pragma once

#include "material.h"

class Skybox
{
  public:
    glm::vec3 Sample(glm::vec3 dir) const;
};

class PointLight
{
  public:
    glm::vec3 position;
    glm::vec3 intensity_rgb;
};

class AreaLight
{
    glm::vec3 p1_, p2_, p3_;
    const Material &material_;
    float area_;

  public:
    AreaLight(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, const Material &mat);
    float GetArea() const;
    float GetNormal() const;

    // first term is source position, the second one is radiance
    std::pair<glm::vec3, glm::vec3> Sample(glm::vec3 target, Sampler &sampler) const;
};