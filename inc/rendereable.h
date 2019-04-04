#pragma once

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <boost/optional.hpp>
#include <glm/glm.hpp>
#include <vector>

struct Intersection
{
    glm::vec3 position;
    glm::vec3 diffuse;
};

struct PointLight
{
    glm::vec3 position;
    glm::vec3 intensity_rgb;
};

struct OpenGLRenderingContext
{
    glm::mat4 vp;

    GLuint mvp_id_;
    GLuint diffuse_id_;

    boost::optional<const std::vector<PointLight> &> lights_;
    glm::vec3 ambient_;
};

struct RaytracingContext
{
    boost::optional<const std::vector<PointLight> &> lights_;
    glm::vec3 ambient_;
};

class Renderable
{
  public:
    virtual void RenderByOpenGL(OpenGLRenderingContext context,
                                struct aiNode *node = nullptr) = 0;
};
