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
    ViewOpenGL vis;

    bool exit_requested = false;
    while (!exit_requested)
    {
        vis.Render();
        vis_rt.Render(vis.GetMVP());

        while (auto action = vis.DequeueAction())
        {
            switch (*action)
            {
            case ViewOpenGL::Exit:
                exit_requested = true;
                break;
            default:
                ASSERT(0, "Action not implemented!")
            }
        }
    }

    log.Info() << "Exit requested. Bye, bye.";
}