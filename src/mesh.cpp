
#include "mesh.h"
#include "exceptions.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

boost::optional<std::pair<glm::vec3, glm::vec3>>
RayIntersectsTriangle(const glm::vec3 orig, const glm::vec3 ray, const glm::vec3 vert0,
                      const glm::vec3 vert1, const glm::vec3 vert2)

{
    glm::vec3 result;

    const glm::vec3 edge1 = vert1 - vert0;
    const glm::vec3 edge2 = vert2 - vert0;
    const glm::vec3 pvec = glm::cross(ray, edge2);
    const float det = glm::dot(edge1, pvec);

    const float epsilon = std::numeric_limits<float>::epsilon();

    if (det > -epsilon && det < epsilon)
        return boost::none;

    const float invDet = 1.0f / det;

    const glm::vec3 tvec = orig - vert0;

    result.x = glm::dot(tvec, pvec) * invDet;
    if (result.x < 0.0f || result.x > 1.0f)
        return boost::none;

    const glm::vec3 qvec = glm::cross(tvec, edge1);

    result.y = glm::dot(ray, qvec) * invDet;
    if (result.y < 0.0f || result.x + result.y > 1.0f)
        return boost::none;

    result.z = glm::dot(edge2, qvec) * invDet;

    // const float abs = std::abs(result.z);
    // const float dist = std::abs(glm::distance(orig, ray));
    // const glm::vec3 intersection = vert0 + result.x * edge2 + result.y * edge1;

    if (result.z < epsilon)
        return boost::none;

    glm::vec3 result_global = vert0 + result.x * edge2 + result.y * edge1;

    return std::pair<glm::vec3, glm::vec3>(
        result_global, glm::vec3(1.0f - (result.x + result.y), result.x, result.y));
}

glm::vec3 GetDiffuse(const Vertex &v1, const Vertex &v2, const Vertex &v3,
                     glm::vec3 bary_cords, Texture &tex)
{
    glm::vec2 uv(0.0f, 0.0f);
    uv += v1.tex_ * bary_cords.x;
    uv += v2.tex_ * bary_cords.y;
    uv += v3.tex_ * bary_cords.z;

    return tex.GetPixel(uv);
}

Mesh::MeshEntry::MeshEntry(std::vector<Vertex> &&Vertices,
                           std::vector<glm::u32> &&Indices, unsigned int MaterialIndex)
    : vertices_(Vertices), indices_(Indices)
{
    glGenBuffers(1, &VB);
    glBindBuffer(GL_ARRAY_BUFFER, VB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices_.size(), &vertices_[0],
                 GL_STATIC_DRAW);

    glGenBuffers(1, &IB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices_.size(),
                 &indices_[0], GL_STATIC_DRAW);

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

    ASSERT(pScene->mNumMeshes == pScene->mNumMaterials);

    for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
    {
        InitMaterial(pScene->mMaterials[i], Dir);
        m_Entries.emplace_back(InitMesh(pScene->mMeshes[i]), materials_.back());
    }

    return true;
}

Mesh::MeshEntry Mesh::InitMesh(const aiMesh *mesh)
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

    return MeshEntry(std::move(Vertices), std::move(Indices), mesh->mMaterialIndex);
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
            materials_.emplace_back(GL_TEXTURE_2D, FullPath);

            log_.Info() << "Loaded texture " << FullPath;
            return;
        }
    }
    else
    {
        log_.Warning() << "No diffuse texture found!";
        materials_.emplace_back(GL_TEXTURE_2D, "res/fail.png");
        return;
    }

    // fixme
    ASSERT(0);
    while (1)
        ;
}

void Mesh::RenderByOpenGL()
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for (auto &obj : m_Entries)
    {
        glBindBuffer(GL_ARRAY_BUFFER, obj.first.VB);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, pos_));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, tex_));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, norm_));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.first.IB);

        const unsigned int MaterialIndex = obj.first.mat_index_;
        obj.second.Bind(GL_TEXTURE0);

        glDrawElements(GL_TRIANGLES, obj.first.indices_.size(), GL_UNSIGNED_INT, 0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

boost::optional<Intersection> Mesh::Raytrace(const glm::vec3 &source,
                                             const glm::vec3 &target)
{
    boost::optional<Intersection> intersection_so_far;
    auto intersection_dist_so_far = std::numeric_limits<float>::infinity();

    for (const auto &obj : m_Entries)
    {
        const auto &vertices = obj.first.vertices_;
        const auto &indices = obj.first.indices_;

        for (int i = 0; i < indices.size(); i += 3)
        {
            auto vertex1 = vertices[indices[i + 0]];
            auto vertex2 = vertices[indices[i + 1]];
            auto vertex3 = vertices[indices[i + 2]];

            glm::vec3 global_position, barycentric;
            if (auto intersection = RayIntersectsTriangle(source, target, vertex1.pos_,
                                                          vertex2.pos_, vertex3.pos_))
            {
                std::tie(global_position, barycentric) = *intersection;

                auto intersection_dist =
                    glm::abs(glm::distance(source, intersection->first));

                if (intersection_dist < intersection_dist_so_far)
                {
                    Intersection in;
                    in.position = intersection->first;
                    in.diffuse = GetDiffuse(vertex1, vertex2, vertex3,
                                            intersection->second, obj.second);

                    intersection_so_far = in;
                    intersection_dist_so_far = intersection_dist;
                }
            }
        }
    }

    return intersection_so_far;
}
