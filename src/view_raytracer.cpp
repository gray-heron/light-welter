
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "config.h"
#include "view_raytracer.h"

using namespace SDL2pp;
using std::get;

glm::vec3 HTN(glm::vec4 homo_vector);
glm::vec4 NTH(glm::vec3 nonhomo_vector);

std::string S(glm::vec3 in)
{
    return "vec3{" + std::to_string(in.x) + ", " + std::to_string(in.y) + ", " +
           std::to_string(in.z) + "}";
}

std::string S(glm::vec4 in)
{
    return "vec4{" + std::to_string(in.x) + ", " + std::to_string(in.y) + ", " +
           std::to_string(in.z) + ", " + std::to_string(in.w) + "}";
}

// https://stackoverflow.com/questions/34255820/save-sdl-texture-to-file
void save_texture(std::string filename, SDL_Renderer *renderer, SDL_Texture *texture)
{
    SDL_Texture *target = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, texture);
    int width, height;
    SDL_QueryTexture(texture, NULL, NULL, &width, &height);
    SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels,
                         surface->pitch);
    IMG_SavePNG(surface, filename.c_str());
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, target);
}

ViewRayCaster::ViewRayCaster(const Scene &scene)
    : rx_(Config::inst().GetOption<int>("resx")),
      ry_(Config::inst().GetOption<int>("resy")),
      window_("RayCaster preview", rx_ + 30, SDL_WINDOWPOS_CENTERED, rx_, ry_,
              Config::inst().GetOption<bool>("interactive") ? 0 : SDL_WINDOW_HIDDEN),
      renderer_(window_, -1, SDL_RENDERER_SOFTWARE),
      tex_(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, rx_, ry_),
      raytracer_surface_(new uint8_t[rx_ * ry_ * 4]),
      sky_color_(Config::inst().GetOption<glm::vec3>("sky")), pathtracer_(scene)
{
}

void ViewRayCaster::Render()
{
    renderer_.Clear();
    renderer_.Copy(tex_, NullOpt, NullOpt);
    renderer_.Present();
}

void ViewRayCaster::TakePicture(glm::vec3 camera_pos, glm::mat4 mvp, const Scene &scene)
{
    Log("RayCasterView").Info() << "Started taking picture.";

    auto inv_mvp = glm::inverse(mvp);
    auto iso = Config::inst().GetOption<float>("iso");

    auto rt_func = [&](int x_start, int cols) -> void {
        for (int x = x_start; x < x_start + cols; x += 1)
        {
            for (unsigned int y = 0; y < ry_; y += 1)
            {
                float xr = (float(x) - float(rx_ / 2)) / (float(rx_ / 2));
                float yr = (float(y) - float(ry_ / 2)) / (float(ry_ / 2));
                glm::vec4 ray_r(xr, -yr, 1, 1);

                auto dir = inv_mvp * ray_r;
                auto intersection = pathtracer_.Trace(camera_pos, glm::normalize(dir));

                uint8_t b;
                uint8_t g;
                uint8_t r;
                uint8_t a;

                if (intersection)
                {
                    auto readout = iso * *intersection; // xd
                    b = float(0xff) * glm::min(readout.x, 1.0f);
                    g = float(0xff) * glm::min(readout.y, 1.0f);
                    r = float(0xff) * glm::min(readout.z, 1.0f);
                    a = 0xff;
                }
                else
                {
                    b = int(sky_color_.x * 255.0f);
                    g = int(sky_color_.y * 255.0f);
                    r = int(sky_color_.z * 255.0f);
                    a = 0xff;
                }

                uint8_t *target_pixel = raytracer_surface_ + y * rx_ * 4 + x * 4;

                *(uint32_t *)target_pixel = r;
                *(uint32_t *)target_pixel += (uint32_t)g << 8;
                *(uint32_t *)target_pixel += (uint32_t)b << 16;
                *(uint32_t *)target_pixel += (uint32_t)a << 24;
            }
        }
    };

    auto threads_num = Config::inst().GetOption<int>("threads");
    auto cols_per_thread = Config::inst().GetOption<int>("cols_per_thread");

    for (unsigned int x = 0; x < rx_; x += threads_num * cols_per_thread)
    {
        std::vector<std::thread> threads;
        for (int t = 0; t < threads_num; t++)
        {
            threads.emplace_back(rt_func, x + t * cols_per_thread, cols_per_thread);
        }

        for (int t = 0; t < threads_num; t++)
        {
            threads[t].join();
        }

        tex_.Update(NullOpt, raytracer_surface_, rx_ * 4);
        renderer_.Clear();
        renderer_.Copy(tex_, NullOpt, NullOpt);
        renderer_.Present();
    }

    Log("RayCasterView").Info() << "Taking picture done. Saving to: "
                                << Config::inst().GetOption<std::string>("target_file");
    save_texture(Config::inst().GetOption<std::string>("target_file"), renderer_.Get(),
                 tex_.Get());
}
