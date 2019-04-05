#pragma once

#include <GL/glew.h>
#include <SDL2pp/Surface.hh>
#include <glm/glm.hpp>
#include <string>

class Texture
{
  public:
    Texture(GLenum texture_target, glm::vec3 diffuse_factor,
            const std::string &file_name);

    ~Texture();
    void Bind(GLenum TextureUnit);
    glm::vec3 GetPixel(glm::vec2 uv) const;
    glm::vec3 diffuse_factor_;

  private:
    GLenum texture_target_;
    GLuint texture_obj_;
    SDL2pp::Surface surface_;
    uint32_t w_, h_;
};
