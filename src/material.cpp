

#include "material.h"
#include "config.h"
#include "exceptions.h"
#include "lights.h"
#include "log.h"
#include "mesh.h"

glm::vec3 GetDiffuse(const Vertex &v1, const Vertex &v2, const Vertex &v3,
                     glm::vec3 bary_cords, const Texture &tex)
{
    glm::vec2 uv(0.0f, 0.0f);
    uv += v1.tex_ * bary_cords.x;
    uv += v2.tex_ * bary_cords.y;
    uv += v3.tex_ * bary_cords.z;

    return tex.GetPixel(uv);
}

MaterialFromAssimp::MaterialFromAssimp(aiMaterial *material, std::string dir)
{
    aiColor3D diff_color;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diff_color);
    diffuse_color_ = glm::vec3(diff_color.r, diff_color.g, diff_color.b);
    parameter_correction_ = Config::inst().GetOption<float>("material_parameter_factor");

    aiColor3D emission_color, reflective_color, specular_color;
    material->Get(AI_MATKEY_COLOR_EMISSIVE, emission_color);
    material->Get(AI_MATKEY_COLOR_AMBIENT, reflective_color);
    material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color);

    reflective_color_ =
        glm::vec3(reflective_color.r, reflective_color.g, reflective_color.b);

    if (!emission_color.IsBlack())
        emission_ = glm::vec3(emission_color.r, emission_color.g, emission_color.b);

    if (!specular_color.IsBlack())
        specular_color_ = glm::vec3(specular_color.r, specular_color.g, specular_color.b);

    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        // fixme: support multiple textures
        STRONG_ASSERT(material->GetTextureCount(aiTextureType_DIFFUSE) == 1);

        aiString path;

        while ((material->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL,
                                     NULL, NULL)))
            ;

        std::string full_path = dir + "/" + path.data;
        texture_ = std::make_unique<Texture>(GL_TEXTURE_2D, full_path);

        Log("Material").Info() << "Loaded texture " << full_path;
        texture_used_ = true;
        return;
    }
    else
    {
        Log("Material").Warning() << "No diffuse texture found!";
        texture_ = std::make_unique<Texture>(GL_TEXTURE_2D, "res/fail.png");
        texture_used_ = false;
        return;
    }
}

glm::vec3 MaterialFromAssimp::BRDF(glm::vec3 from, glm::vec3 p, glm::vec3 to,
                                   glm::vec3 normal, glm::vec3 barycentric,
                                   const Vertex &p1, const Vertex &p2,
                                   const Vertex &p3) const
{
    glm::vec3 kd;
    if (texture_used_)
        kd = GetDiffuse(p1, p2, p3, barycentric, *texture_);
    else
        kd = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::vec3 surface_to_source = from - p;
    glm::vec3 specular_dir =
        2.0f * glm::dot<3, float, glm::qualifier::highp>(normal, surface_to_source) *
            normal -
        surface_to_source;

    float glossy_term =
        std::max(0.0f, glm::dot<3, float, glm::qualifier::highp>(to - p, specular_dir) /
                           glm::length(specular_dir) / glm::length(to - p));

    STRONG_ASSERT(glossy_term <= 1.01f)
    return std::pow(glossy_term, 15.0f) * reflective_color_ * parameter_correction_ +
           diffuse_color_ * kd * parameter_correction_;
}

Material::Reflection MaterialFromAssimp::SampleF(glm::vec3 position, glm::vec3 normal,
                                                 glm::vec3 in_dir, glm::vec3 barycentric,
                                                 const Vertex &p1, const Vertex &p2,
                                                 const Vertex &p3, Sampler &s) const
{
    auto dir = s.SampleDirection(normal);

    return {.radiance_ = BRDF(position + in_dir, position, position + dir, normal,
                              barycentric, p1, p2, p3),
            .pdf_ = 1.0f / (2.0f * glm::pi<float>()),
            .dir_ = dir};
}

Material::Reflection
MaterialFromAssimp::SampleSpecular(glm::vec3 position, glm::vec3 normal, glm::vec3 in_dir,
                                   glm::vec3 barycentric, const Vertex &p1,
                                   const Vertex &p2, const Vertex &p3, Sampler &s) const
{
    glm::vec3 specular_dir =
        2.0f * glm::dot<3, float, glm::qualifier::highp>(normal, -in_dir) * normal -
        -in_dir;

    return {.radiance_ = *specular_color_ * parameter_correction_,
            .pdf_ = 1.0f,
            .dir_ = specular_dir};
}

void MaterialFromAssimp::SetupForOpenGL()
{
    if (texture_)
        texture_->SetupForOpenGL();
}

void MaterialFromAssimp::BindForOpenGL(const OpenGLRenderingContext &context,
                                       GLenum texture_unit)
{
    glUniform3f(context.diffuse_id_, diffuse_color_.x, diffuse_color_.y,
                diffuse_color_.z);

    if (texture_)
        texture_->Bind(texture_unit);
    else
    {
        STRONG_ASSERT(false);
    }
}

bool MaterialFromAssimp::IsEmissive() const { return emission_.is_initialized(); }

glm::vec3 MaterialFromAssimp::Emission() const
{
    if (emission_)
        return *emission_;
    else
        return glm::vec3(0.0f, 0.0f, 0.0f);
}

bool MaterialFromAssimp::HasSpecular() const { return specular_color_.is_initialized(); }