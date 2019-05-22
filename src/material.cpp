

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

    aiColor3D emission_color;
    material->Get(AI_MATKEY_COLOR_EMISSIVE, emission_color);

    if (material->GetTextureCount(aiTextureType_EMISSIVE) || !emission_color.IsBlack())
    {
        emission_ = glm::vec3(emission_color.r, emission_color.g, emission_color.b);
    }

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

    return diffuse_color_ * parameter_correction_ * kd;
}

Material::Reflection MaterialFromAssimp::SampleF(glm::vec3 position, glm::vec3 normal,
                                                 glm::vec3 in_dir, glm::vec3 barycentric,
                                                 const Vertex &p1, const Vertex &p2,
                                                 const Vertex &p3, Sampler &s) const
{
    auto dir = s.SampleDirection(normal);

    return {.radiance_ = BRDF(position + in_dir, position, position + dir, normal,
                              barycentric, p1, p2, p3),
            .is_specular_ = false,
            .pdf_ = 1.0f,
            .dir_ = dir};
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