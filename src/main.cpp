#include <stdio.h>

#include "config.h"
#include "log.h"
#include "view_opengl.h"
#include "view_raytracer.h"

using std::string;

int main(int argc, char **argv)
{
    SDL2pp::SDL sdl_(SDL_INIT_VIDEO);

    Log log("main");
    log.Info() << "Ray tracer demo";

    Config::inst().Load(argc, argv);

    auto config_path = Config::inst().GetOption<std::string>("config");
    if (config_path != "")
    {
        Config::inst().Load(config_path);
    }

    LoggingSingleton::inst().SetConsoleVerbosity(
        Config::inst().GetOption<bool>("verbose"));
    LoggingSingleton::inst().AddLogFile(
        Config::inst().GetOption<std::string>("log_file"));

    Config::inst().DumpSettings();

    //====================

    ViewRaytracer vis_rt;
    ViewOpenGL vis_gl;
    Scene scene;

    scene.ambient_light_ = {0.15f, 0.15f, 0.15f};
    scene.renderables_.push_back(new Mesh("res/phoenix/phoenix_ugv.md2"));
    // scene.renderables_.push_back(new Mesh("res/view_test/cornell_box.obj"));
    /*
        scene.point_lights_.push_back({
            glm::vec3(278.0f, 448.0f, 279.5f),
            glm::vec3(0.7f, 0.7f, 0.7f),
        });
        */
    scene.point_lights_.push_back({
        glm::vec3(100.0f, 220.0f, 100.5f),
        glm::vec3(0.8f, 0.8f, 0.8f),
    });

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
            default:
                ASSERT(0, "Action not implemented!")
            }
        }
    }

    log.Info() << "Exit requested. Bye, bye.";
}