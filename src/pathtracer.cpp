
#include "pathtracer.h"

PathTracer::PathTracer(const Scene &scene)
    : point_lights_(scene.point_lights_), ambient_light_(scene.ambient_light_),
      raycaster_(scene.mesh_)
{
}

boost::optional<glm::vec3> PathTracer::Trace(glm::vec3 camera_pos, glm::vec3 dir)
{
    if (auto intersection = raycaster_.Trace(camera_pos, dir))
    {
        glm::vec3 result;
        result.x = float(intersection->second.object_id_ + 1) / float(10);
        result.y = 1.0f;
        result.z = intersection->second.object_id_ == 0 ? 1.0f : 0.0f;

        return result;
    }
    else
    {
        return boost::none;
    }
}