
#include "pathtracer.h"
#include "spectrum.h"

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
      max_reflections_(Config::inst().GetOption<int>("max_reflections")),
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

glm::vec3 PathTracer::Trace(glm::vec3 camera_pos, glm::vec3 dir) const
{
    Sampler s;
    return Trace(camera_pos, dir, true, glm::vec3(1.0f, 1.0f, 1.0f), s, recursion_level_);
}

boost::optional<int> PathTracer::DebugTrace(glm::vec3 camera_pos, glm::vec3 dir) const
{
    if (auto result = raycaster_.Trace(camera_pos, dir))
        return result->second.object_id_;
    else
        return boost::none;
}

glm::vec3 PathTracer::Trace(glm::vec3 origin, glm::vec3 dir, bool include_emission,
                            glm::vec3 beta, Sampler &sampler, int32_t depth) const
{
    if (depth == -1)
        return glm::vec3();

    // RUSSION RULETTE

    if (auto intersection_raw = raycaster_.Trace(origin, dir))
    {
        glm::vec3 ret(0.0f);
        auto intersection = intersection_raw->first;
        auto surface = intersection_raw->second;
        auto &material = scene_.mesh_->GetMaterial(surface.object_id_);

        float source_cosine = glm::abs(glm::dot<3, float, glm::qualifier::highp>(
            dir, glm::normalize(intersection.normal_)));

        if (include_emission)
            ret += material.Emission() * beta;

        for (const auto &light : scene_.area_lights_)
        {
            auto incoming_light = light.Sample(intersection.global_pos_, sampler);

            float light_cosine = glm::abs(glm::dot<3, float, glm::qualifier::highp>(
                incoming_light.first - intersection.global_pos_,
                glm::normalize(intersection.normal_)));

            auto shadowhit = raycaster_.Trace(
                intersection.global_pos_,
                glm::normalize(incoming_light.first - intersection.global_pos_));

            float dist = glm::length(incoming_light.first - intersection.global_pos_);

            //            STRONG_ASSERT(shadowhit.is_initialized());

            if (shadowhit.is_initialized() && !(shadowhit->first.dist_ < dist))
            {
                float g = light_cosine * source_cosine / (dist * dist);

                ret += material.BRDF(incoming_light.first, intersection.global_pos_,
                                     origin, intersection.normal_) *
                       incoming_light.second * beta * g * light.GetArea();
            }
        }

        for (int i = 0; i < max_reflections_; i++)
        {
            auto reflection = material.SampleF(intersection.global_pos_,
                                               intersection.normal_, dir, sampler);

            glm::vec3 new_beta = beta * reflection.radiance_ / reflection.pdf_;

            ret += Trace(intersection.global_pos_, reflection.dir_,
                         reflection.is_specular_, new_beta, sampler, depth - 1) /
                   float(max_reflections_);
        }

        return ret;
    }
    else
    {
        return scene_.skybox_.Sample(dir);
    }
}