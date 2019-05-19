
#pragma once

#include <string>

#include "lights.h"
#include "log.h"
#include "material.h"
#include "renderable.h"
#include "texture.h"

class Scene;

struct Vertex
{
    glm::vec3 pos_;
    glm::vec2 tex_;
    glm::vec3 norm_;
};

class Mesh : public Renderable
{
  public:
    struct MeshEntry
    {
        MeshEntry(std::vector<Vertex> &&Vertices, std::vector<glm::u32> &&Indices,
                  Material &mat);

        ~MeshEntry();

        void SetupForOpenGL();
        GLuint VB;
        GLuint IB;

        const std::vector<Vertex> vertices_;
        const std::vector<glm::u32> indices_;
        Material &material_;
    };

    Mesh(std::string filename, Scene &scene);
    virtual ~Mesh();

    void RenderByOpenGL(OpenGLRenderingContext context, aiNode *node = nullptr) override;
    void SetupForOpenGL();

    std::vector<MeshEntry> submeshes_;
    Material &GetMaterial(uint32_t obj_index_);

    glm::vec3 GetUpperBound();
    glm::vec3 GetLowerBound();

  private:
    MeshEntry InitMesh(const aiMesh *mesh, std::vector<AreaLight> &lights,
                       const Material &material);

    const aiScene *scene_;

    std::vector<MaterialFromAssimp> materials_;
    Assimp::Importer importer_;
    glm::vec3 lower_bound_;
    glm::vec3 upper_bound_;

    Log log_{"Mesh"};
};