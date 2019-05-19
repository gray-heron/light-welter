
#pragma once

#include <GL/glew.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <list>
#include <memory>
#include <vector>

#include "renderable.h"
#include "texture.h"

class Material
{
  public:
    struct Reflection
    {
        glm::vec3 radiance_;
        bool is_specular_;
        float pdf_;
        glm::vec3 dir_;
    };

    virtual glm::vec3 BRDF(glm::vec3 from, glm::vec3 p, glm::vec3 to,
                           glm::vec3 normal) const = 0;

    virtual Reflection SampleF(glm::vec3 position, glm::vec3 normal, glm::vec3 in_dir,
                               Sampler &s) const = 0;

    virtual glm::vec3 Emission() const = 0;
    virtual bool IsEmissive() const = 0;

    virtual void SetupForOpenGL() = 0;
    virtual void BindForOpenGL(const OpenGLRenderingContext &context,
                               GLenum texture_unit) = 0;

    virtual ~Material() = default;
};

class MaterialFromAssimp : public Material
{
    std::unique_ptr<Texture> texture_;
    glm::vec3 diffuse_color_;
    boost::optional<glm::vec3> emission_;

  public:
    MaterialFromAssimp(aiMaterial *mat, std::string dir);

    virtual glm::vec3 BRDF(glm::vec3 from, glm::vec3 p, glm::vec3 to,
                           glm::vec3 normal) const override;

    virtual Reflection SampleF(glm::vec3 position, glm::vec3 normal, glm::vec3 in_dir,
                               Sampler &s) const override;

    virtual glm::vec3 Emission() const override;

    virtual void BindForOpenGL(const OpenGLRenderingContext &context,
                               GLenum texture_unit) override;

    virtual void SetupForOpenGL() override;
    bool IsEmissive() const override;
};
