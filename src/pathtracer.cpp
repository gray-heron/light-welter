
#include "pathtracer.h"

glm::vec3 GetDiffuse(const Vertex &v1, const Vertex &v2, const Vertex &v3,
                     glm::vec3 bary_cords, const Texture &tex)
{
    glm::vec2 uv(0.0f, 0.0f);
    uv += v1.tex_ * bary_cords.x;
    uv += v2.tex_ * bary_cords.y;
    uv += v3.tex_ * bary_cords.z;

    return tex.GetPixel(uv);
}

PathTracer::PathTracer(const Scene &scene)
    : scene_(scene), raycaster_(scene.mesh_),
      recursion_level_(Config::inst().GetOption<int>("recursion")),
      specular_reflection_factor_(
          Config::inst().GetOption<float>("specular_reflection_factor"))
{
}

glm::vec3 PathTracer::FIL(boost::optional<glm::vec3> light) const
{
    if (light)
        return *light;
    else
        return glm::vec3(0.0f, 0.0f, 0.0f);
}

boost::optional<glm::vec3> PathTracer::Trace(glm::vec3 camera_pos, glm::vec3 dir) const
{
    return Trace(camera_pos, dir, recursion_level_);
}

boost::optional<glm::vec3> PathTracer::Trace(glm::vec3 origin, glm::vec3 dir,
                                             int32_t depth) const
{
    if (depth == -1)
        return boost::none;

    if (auto intersection = raycaster_.Trace(origin, dir))
    {
        glm::vec3 out_emission = glm::vec3(scene_.ambient_light_);

        TriangleIndices indices(intersection->second);
        TriangleIntersection triangle_intersection(intersection->first);

        const auto &vertices =
            scene_.mesh_->m_Entries[indices.object_id_].first.vertices_;
        const auto &texture = scene_.mesh_->m_Entries[indices.object_id_].second;

        // diffuse
        for (const auto &light : scene_.point_lights_)
        {
            float dist_to_light =
                glm::length(light.position - triangle_intersection.global_pos_);

            if (auto shadowhit = raycaster_.Trace(
                    triangle_intersection.global_pos_,
                    glm::normalize(light.position - triangle_intersection.global_pos_)))
                if (shadowhit->first.dist_ < dist_to_light)
                    continue;

            auto light_to_intersection =
                glm::normalize(light.position - triangle_intersection.global_pos_);

            auto angle_factor = glm::abs(glm::dot<3, float, glm::qualifier::highp>(
                light_to_intersection, glm::normalize(triangle_intersection.normal_)));

            auto distance_factor = 1.0f / (dist_to_light * dist_to_light);

            out_emission += angle_factor * distance_factor * light.intensity_rgb;
        }

        if (texture)
        {
            out_emission *= GetDiffuse(vertices[indices.t1_], vertices[indices.t2_],
                                       vertices[indices.t3_],
                                       triangle_intersection.barycentric_pos_, *texture);
        }

        // specular
        if (depth > 0)
        {
            auto surface_to_source =
                glm::normalize(origin - triangle_intersection.global_pos_);

            auto surface_emission_dir =
                2.0f *
                    glm::dot<3, float, glm::qualifier::highp>(
                        triangle_intersection.normal_, surface_to_source) *
                    triangle_intersection.normal_ -
                surface_to_source;

            out_emission += FIL(Trace(triangle_intersection.global_pos_,
                                      surface_emission_dir, depth - 1)) *
                            specular_reflection_factor_;
        }

        return out_emission;
    }
    else
    {
        return boost::none;
    }
}