
#pragma once

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <boost/optional.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "lights.h"
#include "mesh.h"
#include "spectrum.h"

class Scene
{
  public:
    std::shared_ptr<Mesh> mesh_;
    std::vector<PointLight> point_lights_;
    std::vector<AreaLight> area_lights_;
    glm::vec3 ambient_light_;

    Skybox skybox_;
};