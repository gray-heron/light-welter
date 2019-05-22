
#include "lights.h"
#include "config.h"

AreaLight::AreaLight(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, const Material &mat)
    : p1_(p1), p2_(p2), p3_(p3), material_(mat)
{
    area_ = glm::abs(glm::length(glm::cross(p2_ - p1_, p3_ - p1_))) / 2.0f;
};

std::pair<glm::vec3, glm::vec3> AreaLight::Sample(glm::vec3 target,
                                                  Sampler &sampler) const
{
    float a, b;
    std::tie(a, b) = sampler.SamplePair();
    if (a + b > 1.0f)
    {
        a = 1.0f - a;
        b = 1.0f - b;
    }

    return std::make_pair(p1_ + (p2_ - p1_) * a + (p3_ - p1_) * b, material_.Emission());
}

float AreaLight::GetArea() const { return area_; }

Skybox::Skybox() : radiance_(Config::inst().GetOption<glm::vec3>("sky")) {}

glm::vec3 Skybox::Sample(glm::vec3) const { return radiance_; }