

#include <SDL2/SDL_surface.h>
#include <boost/filesystem.hpp>

#include "exceptions.h"
#include "texture.h"

glm::vec3 GetPixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
    case 1:
        return glm::vec3(float(*p) / float(0xff), float(*p) / float(0xff),
                         float(*p) / float(0xff));
        break;

    case 2:
        STRONG_ASSERT(0);
        break;

    case 3:
        return glm::vec3(float(p[0]) / float(0xff), float(p[1]) / float(0xff),
                         float(p[2]) / float(0xff));
        break;

    case 4:
        return glm::vec3(float(p[0]) / float(0xff), float(p[1]) / float(0xff),
                         float(p[2]) / float(0xff));
        break;

    default:
        STRONG_ASSERT(0);
        return glm::vec3();
    }
}

Texture::Texture(GLenum texture_target, glm::vec3 diff_color,
                 const std::string &file_name)
    : diffuse_factor_(diff_color), texture_target_(texture_target),
      surface_(boost::filesystem::exists(file_name) ? SDL2pp::Surface(file_name)
                                                    : SDL2pp::Surface("res/fail.png")),
      w_(surface_.GetWidth()), h_(surface_.GetHeight())
{
    if (!boost::filesystem::exists(file_name))
        Log("TextureReader").Error() << "Texture: " << file_name << " does not exist!";

    STRONG_ASSERT(surface_.GetSize().x > 0);

    glGenTextures(1, &texture_obj_);
    glBindTexture(texture_target, texture_obj_);
}

void Texture::SetupForOpenGL()
{
    int mode;
    switch (surface_.Get()->format->BytesPerPixel)
    {
    case 3:
        mode = GL_RGB;
        break;
    case 4:
        mode = GL_RGBA;
        break;

    default:
        STRONG_ASSERT(0);
    }

    glBindTexture(texture_target_, texture_obj_);

    glTexImage2D(texture_target_, 0, mode, surface_.GetWidth(), surface_.GetHeight(), 0,
                 mode, GL_UNSIGNED_BYTE, surface_.Get()->pixels);

    glTexParameterf(texture_target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(texture_target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(texture_target_, 0);
}

void Texture::Bind(GLenum texture_unit)
{
    glActiveTexture(texture_unit);
    glBindTexture(texture_target_, texture_obj_);
}

glm::vec3 Texture::GetPixel(glm::vec2 uv) const
{
    int x = uv.x * w_;
    int y = uv.y * h_;
    return ::GetPixel(surface_.Get(), x, y) * diffuse_factor_;
}

Texture::~Texture()
{
    // fixme
}