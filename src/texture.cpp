
#include <SDL2/SDL_surface.h>
#include <SDL2pp/Surface.hh>
#include <boost/filesystem.hpp>

#include "exceptions.h"
#include "texture.h"

Texture::Texture(GLenum texture_target, const std::string &file_name)
    : texture_target_(texture_target)
{
    SDL2pp::Surface surface("res/fail.png");

    if (boost::filesystem::exists(file_name))
    {
        surface = SDL2pp::Surface(file_name);
    }
    else
    {
        Log("TextureReader").Error() << "Texture: " << file_name << " does not exist!";
    }

    ASSERT(surface.GetSize().x > 0);

    GLuint txID;
    glGenTextures(1, &texture_obj_);
    glBindTexture(texture_target, texture_obj_);

    int mode;
    switch (surface.Get()->format->BytesPerPixel)
    {
    case 3:
        mode = GL_RGB;
        break;
    case 4:
        mode = GL_RGBA;
        break;

    default:
        ASSERT(0);
    }

    glBindTexture(texture_target_, texture_obj_);

    glTexImage2D(texture_target_, 0, mode, surface.GetWidth(), surface.GetHeight(), 0,
                 mode, GL_UNSIGNED_BYTE, surface.Get()->pixels);

    glTexParameterf(texture_target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(texture_target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(texture_target_, 0);
}

void Texture::Bind(GLenum texture_unit)
{
    glActiveTexture(texture_unit);
    glBindTexture(texture_target_, texture_obj_);
}

Texture::~Texture()
{
    // fixme
}