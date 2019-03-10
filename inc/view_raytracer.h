
#pragma once
#include <tuple>

#include <SDL2pp/SDL2pp.hh>
#include <boost/optional/optional.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "log.h"
#include "mesh.h"

class ViewRaytracer
{
  public:
    enum Action
    {
        Exit
    };

  private:
    const uint32_t rx_, ry_;

    SDL2pp::Window window_;
    SDL2pp::Renderer renderer_;
    SDL2pp::Texture tex_;

    uint8_t *raytracer_surface_;

    boost::optional<Mesh> mesh_;

  public:
    ViewRaytracer();
    glm::mat4 GetMVP();

    bool Render(glm::mat4 mvp);
    Log log_{"ViewRaytracer"};
};