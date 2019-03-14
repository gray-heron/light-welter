
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
    glm::vec3 ambient;
};

class Renderable
{
  public:
    virtual void RenderByOpenGL(OpenGLRenderingContext context,
                                struct aiNode *node = nullptr) = 0;

    virtual boost::optional<Intersection> Raytrace(const glm::vec3 &source,
                                                   const glm::vec3 &target,
                                                   const OpenGLRenderingContext &context,
                                                   int recursion_depth) = 0;
};
class Scene
{
  public:
    std::vector<Renderable *> renderables_;
    std::vector<PointLight> point_lights_;

    glm::vec3 ambient_light_;
};