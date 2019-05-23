
#include "pathtracer.h"
#include "spectrum.h"

extern std::string S(glm::vec4 in);
extern std::string S(glm::vec3 in);

PathTracer::PathTracer(const Scene &scene)
    : scene_(scene), raycaster_(scene.mesh_),
      recursion_level_(Config::inst().GetOption<int>("recursion")),
      max_reflections_(Config::inst().GetOption<int>("max_reflections")),
      roulette_factor_(Config::inst().GetOption<float>("roulette_factor"))
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
    auto result = Trace(camera_pos, dir);

    log_.Info() << "Randiance from debug ray: " << S(result);

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

    float p = std::max(beta.x, std::max(beta.y, beta.z)) * roulette_factor_;
    p = std::min(1.0f, p);
    if (sampler.Sample() > p)
    {
        // log_.Info() << "Terminating at " << depth;
        return glm::vec3();
    }
    beta *= 1.0f / p;

    // SAMPLE ALL LIGHTS
    if (auto intersection_raw = raycaster_.Trace(origin, dir))
    {
        glm::vec3 ret(0.0f);
        auto intersection = intersection_raw->first;
        auto surface = intersection_raw->second;
        auto &material = scene_.mesh_->GetMaterial(surface.object_id_);
        const auto &vertices = scene_.mesh_->submeshes_[surface.object_id_].vertices_;

        float source_cosine = glm::abs(glm::dot<3, float, glm::qualifier::highp>(
            dir, glm::normalize(intersection.normal_)));

        if (include_emission)
            ret += material.Emission() * beta;

        for (const auto &light : scene_.area_lights_)
        {
            auto incoming_light = light.Sample(intersection.global_pos_, sampler);

            float light_cosine = glm::abs(glm::dot<3, float, glm::qualifier::highp>(
                glm::normalize(incoming_light.first - intersection.global_pos_),
                glm::normalize(intersection.normal_)));

            auto shadowhit = raycaster_.Trace(
                intersection.global_pos_,
                glm::normalize(incoming_light.first - intersection.global_pos_));

            float dist = glm::length(incoming_light.first - intersection.global_pos_);

            // STRONG_ASSERT(shadowhit.is_initialized());

            if (shadowhit.is_initialized() &&
                !(shadowhit->first.dist_ <
                  dist + (32.0f * std::numeric_limits<float>::epsilon())))
            {
                float g = light_cosine * source_cosine /
                          (dist * dist * glm::pi<float>() * glm::pi<float>());

                ret += material.BRDF(incoming_light.first, intersection.global_pos_,
                                     origin, intersection.normal_,
                                     intersection.barycentric_pos_, vertices[surface.t1_],
                                     vertices[surface.t2_], vertices[surface.t3_]) *
                       incoming_light.second * beta * g * light.GetArea();
            }
        }

        // SAMPLE SKY
        glm::vec3 skybox_dir = sampler.SampleDirection(intersection.normal_);
        if (!raycaster_.Trace(intersection.global_pos_, skybox_dir))
        {
            ret += beta * scene_.skybox_.Sample(skybox_dir) *
                   material.BRDF(intersection.global_pos_ + skybox_dir,
                                 intersection.global_pos_, origin, intersection.normal_,
                                 intersection.barycentric_pos_, vertices[surface.t1_],
                                 vertices[surface.t2_], vertices[surface.t3_]);
        }

        // SAMPLE MANY REFLECTIONS
        for (int i = 0; i < max_reflections_; i++)
        {
            auto reflection =
                material.SampleF(intersection.global_pos_, intersection.normal_, dir,
                                 intersection.barycentric_pos_, vertices[surface.t1_],
                                 vertices[surface.t2_], vertices[surface.t3_], sampler);

            glm::vec3 new_beta = beta * reflection.radiance_ / reflection.pdf_;

            ret += Trace(intersection.global_pos_, reflection.dir_, false, new_beta,
                         sampler, depth - 1) /
                   float(max_reflections_);
        }

        if (material.HasSpecular())
        {
            auto reflection = material.SampleSpecular(
                intersection.global_pos_, intersection.normal_, dir,
                intersection.barycentric_pos_, vertices[surface.t1_],
                vertices[surface.t2_], vertices[surface.t3_], sampler);

            glm::vec3 new_beta = beta * reflection.radiance_ / reflection.pdf_;

            ret += Trace(intersection.global_pos_, reflection.dir_, true, new_beta,
                         sampler, depth - 1);
        }

        return ret;
    }
    else
    {
        return beta * scene_.skybox_.Sample(dir);
    }
}