
#include <SDL2/SDL.h>

#include "config.h"
#include "view_raytracer.h"

using namespace SDL2pp;
using std::get;

std::string S(glm::vec4 in)
{
    return "vec4{" + std::to_string(in.x) + ", " + std::to_string(in.y) + ", " +
           std::to_string(in.z) + ", " + std::to_string(in.w) + "}";
}

ViewRaytracer::ViewRaytracer()
    : rx_(Config::inst().GetOption<int>("resx")),
      ry_(Config::inst().GetOption<int>("resy")),
      window_("Raytracer preview", rx_ + 30, SDL_WINDOWPOS_CENTERED, rx_, ry_, 0),
      renderer_(window_, -1, SDL_RENDERER_SOFTWARE),
      tex_(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, rx_, ry_)
{
    raytracer_surface_ = new uint8_t[rx_ * ry_ * 4];

    for (int x = 0; x < rx_; x++)
    {
        for (int y = 0; y < ry_; y++)
        {
            uint8_t b = x;
            uint8_t g = y;
            uint8_t r = 0x0;
            uint8_t a = 0xff;

            uint8_t *target_pixel = raytracer_surface_ + y * rx_ * 4 + x * 4;

            *(uint32_t *)target_pixel = r;
            *(uint32_t *)target_pixel += (uint32_t)g << 8;
            *(uint32_t *)target_pixel += (uint32_t)b << 16;
            *(uint32_t *)target_pixel += (uint32_t)a << 24;
        }
    }

    tex_.Update(NullOpt, raytracer_surface_, rx_ * 4);
}

bool ViewRaytracer::Render(glm::mat4 mvp)
{
    glm::vec4 middle(-1, 0, 0.99, 1);
    auto t = glm::inverse(mvp) * middle;
    t /= t.w;
    // t /= glm::length(t);
    log_.Info() << S(t);

    renderer_.Clear();
    renderer_.Copy(tex_, NullOpt, NullOpt);
    renderer_.Present();
}
