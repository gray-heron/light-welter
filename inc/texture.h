#pragma once

#include <GL/glew.h>
#include <string>

class Texture
{
  public:
    Texture(GLenum texture_target, const std::string &file_name);

    ~Texture();
    void Bind(GLenum TextureUnit);

  private:
    GLenum texture_target_;
    GLuint texture_obj_;
};
