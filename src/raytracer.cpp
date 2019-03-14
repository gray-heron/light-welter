

#include "raytracer.h"

boost::optional<Intersection> Raytracer::Trace(glm::vec3 source, glm::vec3 target,
                                               const Scene &scene)
{
    boost::optional<Intersection> intersection_so_far;
    // auto intersection_dist_so_far = std::numeric_limits<float>::infinity();
    OpenGLRenderingContext ctx;
    ctx.lights_ = scene.point_lights_;
    ctx.ambient = scene.ambient_light_;

    for (auto renderable : scene.renderables_)
    {
        if (auto intersection = renderable->Raytrace(source, target, ctx, 1))
        {
            // auto intersection_dist = glm::length(source - intersection->position);
            // if (intersection_dist < intersection_dist_so_far)
            //{
            intersection_so_far = intersection;
            // intersection_dist_so_far = intersection_dist;
            // }
            break;
        }
    }

    return intersection_so_far;
}