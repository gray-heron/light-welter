
#pragma once
#include <GL/glew.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include <string>

#include "log.h"
#include "texture.h"

struct Vertex
{
    glm::vec3 pos_;
    glm::vec2 tex_;
    glm::vec3 norm_;
};

class Mesh
{
  public:
    Mesh(std::string filename);
    ~Mesh();

    void Render();

  private:
    bool InitFromScene(const aiScene *pScene, const std::string &Filename);
    void InitMesh(const aiMesh *mesh);
    void InitMaterial(const aiMaterial *material, std::string dir);
    void Clear();

    struct MeshEntry
    {
        MeshEntry(const std::vector<Vertex> &Vertices,
                  const std::vector<glm::u32> &Indices, unsigned int MaterialIndex);

        ~MeshEntry();

        GLuint VB;
        GLuint IB;
        unsigned int num_indices_;
        unsigned int mat_index_;
    };

    std::vector<MeshEntry> m_Entries;
    std::vector<Texture> m_Textures;

    Log log_{"Mesh"};
};