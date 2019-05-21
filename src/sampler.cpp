
#include "sampler.h"

Sampler::Sampler() : mt_(rd_()), dist_(0.0f, 1.0f) {}

float Sampler::Sample() { return dist_(mt_); }

std::pair<float, float> Sampler::SamplePair()
{
    return std::make_pair(Sample(), Sample());
}

glm::vec3 Sampler::SampleDirection()
{
    return glm::normalize(glm::vec3(2.0f * Sample() - 1.0f, 2.0f * Sample() - 1.0f,
                                    2.0f * Sample() - 1.0f));
}

glm::vec3 Sampler::SampleDirection(glm::vec3 normal)
{
    while (true)
    {
        auto dir = SampleDirection();
        if (dir.x * normal.x + dir.y * normal.y + dir.z * normal.z > 0)
        {
            return dir;
        }
    }
}