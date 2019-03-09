#include <stdio.h>

#include "config.h"
#include "log.h"
#include "visualisation.h"

using std::string;

int main(int argc, char **argv)
{
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

    Visualisation vis;
    bool exit_requested = false;
    while (!exit_requested)
    {
        vis.Render();

        while (auto action = vis.DequeueAction())
        {
            switch (*action)
            {
            case Visualisation::Exit:
                exit_requested = true;
                break;
            default:
                ASSERT(0, "Action not implemented!")
            }
        }
    }

    log.Info() << "Exit requested. Bye, bye.";
}