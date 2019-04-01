
#pragma once
#include <tuple>

#include <SDL2pp/SDL2pp.hh>
#include <boost/optional/optional.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <queue>

#include "log.h"
#include "mesh.h"
#include "shader.h"

class ViewOpenGL
{
  public:
    enum Action
    {
        Exit,
        TakePicture,
        OneShot
    };

  private:
    const uint32_t rx_, ry_;

    SDL2pp::Window window_;
    SDL_GLContext main_context_;

    glm::vec3 camera_pos_;
    float fov_;
    float pitch_, yaw_;

    std::queue<Action> action_queue_;

    void HandleKeyDown(SDL_KeyboardEvent key);
    void HandleMouseKeyDown(SDL_MouseButtonEvent btn);
    glm::mat4 UpdateCamera();

    OpenGLRenderingContext rendering_context_;

  public:
    boost::optional<glm::vec3> alt_look_at_;
    glm::vec3 camera_lookat_;

    ViewOpenGL();
    glm::mat4 GetMVP();
    glm::vec3 GetCameraPos();

    void Render(const Scene &scene);
    Log log_{"ViewOpenGL"};

    boost::optional<ViewOpenGL::Action> DequeueAction();
};