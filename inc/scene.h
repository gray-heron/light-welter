
#pragma once

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <boost/optional.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "mesh.h"

class Scene
{
  public:
    std::unique_ptr<Mesh> mesh_;
    std::vector<PointLight> point_lights_;

    glm::vec3 ambient_light_;
};