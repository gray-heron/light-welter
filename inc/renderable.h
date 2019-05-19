#pragma once

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <boost/optional.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "material.h"
#include "sampler.h"

struct Intersection
{
    glm::vec3 position;
    glm::vec3 diffuse;
};

struct OpenGLRenderingContext
{
    glm::mat4 vp;

    GLuint mvp_id_;
    GLuint diffuse_id_;

    glm::vec3 ambient_;
};

class Renderable
{
  public:
    virtual void RenderByOpenGL(OpenGLRenderingContext context,
                                struct aiNode *node = nullptr) = 0;
};
