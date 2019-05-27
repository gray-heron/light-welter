
#include "sampler.h"

Sampler::Sampler() : mt_(rd_()), dist_(0.0f, 1.0f) {}

float Sampler::Sample() { return dist_(mt_); }

std::pair<float, float> Sampler::SamplePair()
{
    return std::make_pair(Sample(), Sample());
}

glm::vec3 Sampler::SampleDirection()
{
    float theta = 2.0f * M_PI * Sample();
    float phi = glm::acos(1.0f - 2.0f * Sample());
    return glm::vec3(glm::sin(phi) * glm::cos(theta), glm::sin(phi) * glm::sin(theta),
                     glm::cos(phi));
}

glm::vec3 Sampler::SampleDirection(glm::vec3 normal)
{
    auto dir = SampleDirection();
    if (dir.x * normal.x + dir.y * normal.y + dir.z * normal.z > 0)
    {
        return dir;
    }
    else
    {
        return -dir;
    }
}