
#pragma once
#include <GL/glew.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <list>
#include <memory>
#include <vector>

#include <string>

#include "log.h"
#include "scene.h"
#include "texture.h"

struct Vertex
{
    glm::vec3 pos_;
    glm::vec2 tex_;
    glm::vec3 norm_;
};

class Mesh : public Renderable
{
  public:
    Mesh(std::string filename);
    ~Mesh();

    void RenderByOpenGL(OpenGLRenderingContext context, aiNode *node = nullptr) override;
    boost::optional<Intersection> Raytrace(const glm::vec3 &source,
                                           const glm::vec3 &target) override;

  private:
    struct MeshEntry
    {
        MeshEntry(std::vector<Vertex> &&Vertices, std::vector<glm::u32> &&Indices,
                  unsigned int MaterialIndex);

        ~MeshEntry();

        GLuint VB;
        GLuint IB;
        unsigned int mat_index_;

        const std::vector<Vertex> vertices_;
        const std::vector<glm::u32> indices_;
    };

    bool InitFromScene(const aiScene *pScene, const std::string &Filename);
    MeshEntry InitMesh(const aiMesh *mesh);
    void InitMaterial(int index, const aiMaterial *material, std::string dir);

    std::unique_ptr<boost::optional<Texture>[]> materials_;
    std::vector<std::pair<MeshEntry, boost::optional<Texture> &>> m_Entries;

    const aiScene *scene_;
    Assimp::Importer Importer;

    Log log_{"Mesh"};
};