
#include <SDL2/SDL.h>

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

ViewRaytracer::ViewRaytracer()
    : rx_(Config::inst().GetOption<int>("resx")),
      ry_(Config::inst().GetOption<int>("resy")),
      window_("Raytracer preview", rx_ * 2 + 30, SDL_WINDOWPOS_CENTERED, rx_, ry_, 0),
      renderer_(window_, -1, SDL_RENDERER_SOFTWARE),
      tex_(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, rx_, ry_),
      raytracer_surface_(new uint8_t[rx_ * ry_ * 4]), raytracer_()
{
}

void ViewRaytracer::Render()
{
    renderer_.Clear();
    renderer_.Copy(tex_, NullOpt, NullOpt);
    renderer_.Present();
}

void ViewRaytracer::TakePicture(glm::vec3 camera_pos, glm::mat4 mvp, const Scene &scene)
{
    Log("RaytracerView").Info() << "Started taking picture.";

    auto inv_mvp = glm::inverse(mvp);

    auto rt_func = [&](int x_start, int cols) -> void {
        for (int x = x_start; x < x_start + cols; x += 1)
        {
            for (int y = 0; y < ry_; y += 1)
            {
                float xr = (float(x) - float(rx_ / 2)) / (float(rx_ / 2));
                float yr = (float(y) - float(ry_ / 2)) / (float(ry_ / 2));
                glm::vec4 ray_r(xr, -yr, 1, 1);

                auto target = inv_mvp * ray_r;
                auto intersection = raytracer_.Trace(camera_pos, target, scene);

                uint8_t b;
                uint8_t g;
                uint8_t r;
                uint8_t a;

                if (intersection)
                {
                    b = float(0xff) * glm::min(intersection->diffuse.x, 1.0f);
                    g = float(0xff) * glm::min(intersection->diffuse.y, 1.0f);
                    r = float(0xff) * glm::min(intersection->diffuse.z, 1.0f);
                    a = 0xff;
                }
                else
                {
                    b = 0;
                    g = 0;
                    r = 0;
                    a = 0;
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

    for (int x = 0; x < rx_; x += threads_num * cols_per_thread)
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

    Log("RaytracerView").Info() << "Taking picture done.";
}
