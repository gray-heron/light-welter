#include <csignal>
#include <stdio.h>

#include "config.h"
#include "log.h"
#include "view_opengl.h"
#include "view_raytracer.h"

using std::string;

int main(int argc, char **argv)
{
    Scene scene;
    SDL2pp::SDL sdl_(SDL_INIT_VIDEO);

    Log log("main");
    log.Info() << "Ray tracer demo";

    Config::inst().Load(argc, argv);

    auto config_path = Config::inst().GetOption<std::string>("config");
    if (config_path != "")
        Config::inst().Load(config_path);

    auto rtc_path = Config::inst().GetOption<std::string>("rtc_file");
    if (rtc_path != "")
        scene.point_lights_ = Config::inst().LoadRTC(rtc_path);

    Config::inst().Load(argc, argv);

    LoggingSingleton::inst().SetConsoleVerbosity(
        Config::inst().GetOption<bool>("verbose"));
    LoggingSingleton::inst().AddLogFile(
        Config::inst().GetOption<std::string>("log_file"));

    Config::inst().DumpSettings();

    //====================

    ViewOpenGL vis_gl;

    if (scene.point_lights_.size() > 0)
        scene.ambient_light_ = {0.000002f, 0.000002f, 0.000002f};
    else
        scene.ambient_light_ = {1.0f, 1.0f, 1.0f};

    scene.mesh_ = std::make_unique<Mesh>(Config::inst().GetOption<string>("scene"));
    // scene.renderables_.push_back(new Mesh("res/view_test/cornell_box.obj"));
    /*
        scene.point_lights_.push_back({
            glm::vec3(278.0f, 448.0f, 279.5f),
            glm::vec3(0.7f, 0.7f, 0.7f),
        });
        */
    // scene.point_lights_.push_back({
    //    glm::vec3(100.0f, 220.0f, 100.5f),
    //    glm::vec3(0.8f, 0.8f, 0.8f),
    //});

    ViewRayCaster vis_rt(scene);

    bool exit_requested = false;
    while (!exit_requested)
    {
        vis_gl.Render(scene);
        vis_rt.Render();

        while (auto action = vis_gl.DequeueAction())
        {
            switch (*action)
            {
            case ViewOpenGL::Exit:
                exit_requested = true;
                break;
            case ViewOpenGL::TakePicture:
                vis_rt.TakePicture(vis_gl.GetCameraPos(), vis_gl.GetMVP(), scene);
                break;
            case ViewOpenGL::OneShot:
            {
                auto inv_mvp = glm::inverse(vis_gl.GetMVP());
                glm::vec4 ray_r(0.0f, 0.0f, 1.0f, 1.0f);

                auto target = inv_mvp * ray_r;
                auto intersection = vis_rt.pathtracer_.Trace(vis_gl.GetCameraPos(),
                                                             glm::normalize(target));
                if (intersection)
                    log.Info() << "Oneshot hit!";
                else
                    log.Info() << "Oneshot miss!";
            }
            break;
            default:
                STRONG_ASSERT(0, "Action not implemented!")
            }
        }
    }

    log.Info() << "Exit requested. Bye, bye.";
}