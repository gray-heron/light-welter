
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
#include "rendereable.h"
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

    Mesh(std::string filename);
    virtual ~Mesh();

    void RenderByOpenGL(OpenGLRenderingContext context, aiNode *node = nullptr) override;

    std::unique_ptr<boost::optional<Texture>[]> materials_;
    std::vector<std::pair<MeshEntry, boost::optional<Texture> &>> m_Entries;

    glm::vec3 GetUpperBound();
    glm::vec3 GetLowerBound();

  private:
    bool InitFromScene(const aiScene *pScene, const std::string &Filename);
    MeshEntry InitMesh(const aiMesh *mesh);
    void InitMaterial(int index, const aiMaterial *material, std::string dir);

    const aiScene *scene_;
    Assimp::Importer importer_;
    glm::vec3 lower_bound_;
    glm::vec3 upper_bound_;

    Log log_{"Mesh"};
};