
#pragma once
#include <tuple>

#include <SDL2pp/SDL2pp.hh>
#include <boost/optional/optional.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <queue>

#include "log.h"
#include "shader.h"

class Visualisation
{
  public:
    enum Action
    {
        Exit
    };

  private:
    SDL2pp::SDL sdl_;
    SDL2pp::Window window_;
    SDL_GLContext main_context_;
    const uint32_t rx_, ry_;
    GLuint mvp_id_;

    glm::vec3 camera_pos_;
    float fov_;
    float pitch_, yaw_;

    glm::vec3 camera_lookat_;

    std::queue<Action> action_queue_;

    void HandleKeyDown(SDL_KeyboardEvent key);
    void HandleMouseKeyDown(SDL_MouseButtonEvent btn);
    glm::mat4 UpdateCamera();

  public:
    Visualisation();

    bool Render();
    Log log_{"Visualisation"};

    boost::optional<Visualisation::Action> DequeueAction();
};