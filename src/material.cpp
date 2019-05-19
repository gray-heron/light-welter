

#include "material.h"
#include "exceptions.h"
#include "lights.h"
#include "log.h"

MaterialFromAssimp::MaterialFromAssimp(aiMaterial *material, std::string dir)
{
    aiColor3D diff_color;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diff_color);
    diffuse_color_ = glm::vec3(diff_color.r, diff_color.g, diff_color.b);

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
        texture_ = std::make_unique<Texture>(GL_TEXTURE_2D, diffuse_color_, full_path);

        Log("Material").Info() << "Loaded texture " << full_path;
        return;
    }
    else
    {
        Log("Material").Warning() << "No diffuse texture found!";
        texture_ =
            std::make_unique<Texture>(GL_TEXTURE_2D, diffuse_color_, "res/fail.png");
        return;
    }
}

glm::vec3 MaterialFromAssimp::BRDF(glm::vec3 from, glm::vec3 p, glm::vec3 to,
                                   glm::vec3 normal) const
{
    return diffuse_color_ * 0.33f;
}

Material::Reflection MaterialFromAssimp::SampleF(glm::vec3 position, glm::vec3 normal,
                                                 glm::vec3 in_dir, Sampler &s) const
{
    while (true)
    {
        auto dir = s.SampleDirection();
        if (dir.x * normal.x + dir.y * normal.y + dir.z * normal.z)
        {
            return {.radiance_ =
                        BRDF(position + in_dir, position, position + dir, normal),
                    .is_specular_ = false,
                    .pdf_ = 1.0f,
                    .dir_ = dir};
        }
    }
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