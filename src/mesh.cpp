
#include "mesh.h"
#include "exceptions.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

Mesh::MeshEntry::MeshEntry(const std::vector<Vertex> &Vertices,
                           const std::vector<glm::u32> &Indices,
                           unsigned int MaterialIndex)
{
    num_indices_ = Indices.size();

    glGenBuffers(1, &VB);
    glBindBuffer(GL_ARRAY_BUFFER, VB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * Vertices.size(), &Vertices[0],
                 GL_STATIC_DRAW);

    glGenBuffers(1, &IB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * num_indices_,
                 &Indices[0], GL_STATIC_DRAW);

    mat_index_ = MaterialIndex;
};

Mesh::MeshEntry::~MeshEntry()
{
    // APPLY RULE OF FIVE
    /*
    glDeleteBuffers(1, &VB);
    glDeleteBuffers(1, &IB);
    */
}

Mesh::Mesh(std::string filename)
{
    Assimp::Importer Importer;

    const aiScene *pScene = Importer.ReadFile(
        filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                              aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

    ASSERT(pScene, "Error parsing " + filename + " : " + Importer.GetErrorString());
    ASSERT(InitFromScene(pScene, filename));
}

Mesh::~Mesh() {}

bool Mesh::InitFromScene(const aiScene *pScene, const std::string &Filename)
{
    for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
    {
        InitMesh(pScene->mMeshes[i]);
    }

    // Extract the directory part from the file name
    std::string::size_type SlashIndex = Filename.find_last_of("/");
    std::string Dir;

    if (SlashIndex == std::string::npos)
    {
        Dir = ".";
    }
    else if (SlashIndex == 0)
    {
        Dir = "/";
    }
    else
    {
        Dir = Filename.substr(0, SlashIndex);
    }

    for (unsigned int i = 0; i < pScene->mNumMaterials; i++)
    {
        InitMaterial(pScene->mMaterials[i], Dir);
    }

    return true;
}

void Mesh::InitMesh(const aiMesh *mesh)
{

    std::vector<Vertex> Vertices;
    std::vector<glm::u32> Indices;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        const aiVector3D *pPos = &(mesh->mVertices[i]);
        const aiVector3D *pNormal = &(mesh->mNormals[i]);
        const aiVector3D *pTexCoord =
            mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][i]) : &Zero3D;

        Vertex v = {glm::vec3(pPos->x, pPos->y, pPos->z),
                    glm::vec2(pTexCoord->x, pTexCoord->y),
                    glm::vec3(pNormal->x, pNormal->y, pNormal->z)};

        Vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace &Face = mesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);
    }

    m_Entries.emplace_back(Vertices, Indices, mesh->mMaterialIndex);
}

void Mesh::InitMaterial(const aiMaterial *material, std::string dir)
{
    // Initialize the materials

    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        aiString Path;

        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL,
                                 NULL) == AI_SUCCESS)
        {
            std::string FullPath = dir + "/" + Path.data;
            m_Textures.emplace_back(GL_TEXTURE_2D, FullPath);

            log_.Info() << "Loaded texture " << FullPath;
        }
    }
    else
    {
        log_.Warning() << "No diffuse texture found!";
        m_Textures.emplace_back(GL_TEXTURE_2D, "white.png");
    }
}

void Mesh::Render()
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for (unsigned int i = 0; i < m_Entries.size(); i++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_Entries[i].VB);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, pos_));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, tex_));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, norm_));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Entries[i].IB);

        const unsigned int MaterialIndex = m_Entries[i].mat_index_;

        if (MaterialIndex < m_Textures.size())
        {
            m_Textures[MaterialIndex].Bind(GL_TEXTURE0);
        }

        glDrawElements(GL_TRIANGLES, m_Entries[i].num_indices_, GL_UNSIGNED_INT, 0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}