
#pragma once

#include <boost/optional.hpp>
#include <glm/glm.hpp>

struct Intersection
{
    glm::vec3 position;
    glm::vec3 diffuse;
};

class Renderable
{
  public:
    virtual void RenderByOpenGL() = 0;

    virtual boost::optional<Intersection> Raytrace(const glm::vec3 &source,
                                                   const glm::vec3 &target) = 0;
};

struct PointLight
{
    glm::vec3 position;
    glm::vec3 intensity_rgb;
};

class Scene
{
  public:
    std::vector<Renderable *> renderables_;
    std::vector<PointLight> point_lights_;

    glm::vec3 ambient_light_;
};