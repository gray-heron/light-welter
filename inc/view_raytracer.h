
#pragma once
#include <tuple>

#include <SDL2pp/SDL2pp.hh>
#include <boost/optional/optional.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <inttypes.h>

#include "log.h"
#include "mesh.h"
#include "pathtracer.h"

class ViewRayCaster
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
    const glm::vec3 sky_color_;
    Log log_{"ViewRayCaster"};

  public:
    ViewRayCaster(const Scene &scene);
    void TakePicture(glm::vec3 camera_pos, glm::mat4 mvp, const Scene &scene);
    void Render();

    PathTracer pathtracer_;
};