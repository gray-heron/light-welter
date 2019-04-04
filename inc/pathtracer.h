#pragma once

#include <boost/optional.hpp>
#include <glm/vec3.hpp>

#include "exceptions.h"
#include "log.h"
#include "raycaster.h"
#include "scene.h"

class PathTracer
{
    Log log_{"PathTracer"};

    glm::vec3 Trace(glm::vec3 source, glm::vec3 target,
                    boost::optional<uint32_t> max_depth = boost::none) const;

    std::vector<PointLight> point_lights_;
    glm::vec3 ambient_light_;
    RayCaster raycaster_;

  public:
    PathTracer(const Scene &scene);

    boost::optional<glm::vec3> Trace(glm::vec3 camera_pos, glm::vec3 dir);
};