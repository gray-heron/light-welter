
#pragma once

#include <glm/glm.hpp>
#include <random>

class Sampler
{
    std::random_device rd_;
    std::mt19937 mt_;
    std::uniform_real_distribution<float> dist_;

  public:
    Sampler();
    float Sample();
    std::pair<float, float> SamplePair();
    glm::vec3 SampleDirection();
};