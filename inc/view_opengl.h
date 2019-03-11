
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
        TakePicture
    };

  private:
    const uint32_t rx_, ry_;

    SDL2pp::Window window_;
    SDL_GLContext main_context_;

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
    ViewOpenGL();
    glm::mat4 GetMVP();
    glm::vec3 GetCameraPos();

    void Render(const Scene &scene);
    Log log_{"ViewOpenGL"};

    boost::optional<ViewOpenGL::Action> DequeueAction();
};