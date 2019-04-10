#pragma once

#include <boost/optional.hpp>
#include <glm/glm.hpp>

#include "config.h"
#include "exceptions.h"
#include "log.h"
#include "raycaster.h"
#include "scene.h"

class PathTracer
{
    Log log_{"PathTracer"};

    boost::optional<glm::vec3> Trace(glm::vec3 source, glm::vec3 target,
                                     int32_t depth) const;

    // Forced Incomming Light FIXME
    glm::vec3 FIL(boost::optional<glm::vec3> light) const;

    const Scene scene_;
    const RayCaster raycaster_;
    const int recursion_level_;
    const float specular_reflection_factor_;

  public:
    PathTracer(const Scene &scene);

    boost::optional<glm::vec3> Trace(glm::vec3 camera_pos, glm::vec3 dir) const;
    boost::optional<int> DebugTrace(glm::vec3 camera_pos, glm::vec3 dir) const;
};