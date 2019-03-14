
#include "mesh.h"
#include "exceptions.h"

// https://gamedev.stackexchange.com/questions/133109/m%C3%B6ller-trumbore-false-positive
// dist, intersection pos, barycentric, normal
boost::optional<std::tuple<float, glm::vec3, glm::vec3, glm::vec3>>
RayIntersectsTriangle(const glm::vec3 orig, const glm::vec3 ray, const glm::vec3 vert0,
                      const glm::vec3 vert1, const glm::vec3 vert2)

{
    glm::vec2 result;

    const glm::vec3 edge1 = vert1 - vert0;
    const glm::vec3 edge2 = vert2 - vert0;
    const glm::vec3 pvec = glm::cross(ray, edge2);
    const float det = glm::dot(edge1, pvec);

    const float epsilon = 32.0f * std::numeric_limits<float>::epsilon();

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

    float t = glm::dot(edge2, qvec) * invDet;

    if (t < epsilon)
        return boost::none;

    // glm::vec3 intersection = vert0 + result.x * edge2 + result.y * edge1;
    glm::vec3 intersection = orig + ray * t;

    auto normal = glm::normalize(glm::cross(edge1, edge2));

    return std::tuple<float, glm::vec3, glm::vec3, glm::vec3>(
        t, intersection, glm::vec3(1.0f - (result.x + result.y), result.x, result.y),
        normal);
}

// https://stackoverflow.com/questions/29184311/how-to-rotate-a-skinned-models-bones-in-c-using-assimp
inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 &from)
{
    glm::mat4 to;

    to[0][0] = (GLfloat)from.a1;
    to[0][1] = (GLfloat)from.b1;
    to[0][2] = (GLfloat)from.c1;
    to[0][3] = (GLfloat)from.d1;
    to[1][0] = (GLfloat)from.a2;
    to[1][1] = (GLfloat)from.b2;
    to[1][2] = (GLfloat)from.c2;
    to[1][3] = (GLfloat)from.d2;
    to[2][0] = (GLfloat)from.a3;
    to[2][1] = (GLfloat)from.b3;
    to[2][2] = (GLfloat)from.c3;
    to[2][3] = (GLfloat)from.d3;
    to[3][0] = (GLfloat)from.a4;
    to[3][1] = (GLfloat)from.b4;
    to[3][2] = (GLfloat)from.c4;
    to[3][3] = (GLfloat)from.d4;

    return to;
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
    materials_ = std::make_unique<boost::optional<Texture>[]>(pScene->mNumMaterials);

    log_.Info() << "Scene has textures? " << pScene->HasTextures();
    log_.Info() << "Scene has lights? " << pScene->HasLights();

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
        InitMaterial(i, pScene->mMaterials[i], Dir);
    }

    for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
    {
        m_Entries.emplace_back(InitMesh(pScene->mMeshes[i]),
                               materials_[pScene->mMeshes[i]->mMaterialIndex]);
    }

    scene_ = pScene;

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

void Mesh::InitMaterial(int index, const aiMaterial *material, std::string dir)
{
    aiColor3D diff_color;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diff_color);
    auto glm_diff_color = glm::vec3(diff_color.r, diff_color.g, diff_color.b);

    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        aiString Path;

        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL,
                                 NULL) == AI_SUCCESS)
        {
            std::string FullPath = dir + "/" + Path.data;
            materials_[index].emplace(GL_TEXTURE_2D, glm_diff_color, FullPath);

            log_.Info() << "Loaded texture " << FullPath;
            return;
        }
    }
    else
    {
        log_.Warning() << "No diffuse texture found!";
        materials_[index].emplace(GL_TEXTURE_2D, glm_diff_color, "res/fail.png");
        return;
    }

    // fixme
    ASSERT(0);
    while (1)
        ;
}

void Mesh::RenderByOpenGL(OpenGLRenderingContext context, aiNode *node)
{
    if (node == nullptr)
        node = scene_->mRootNode;

    context.vp = context.vp * aiMatrix4x4ToGlm(node->mTransformation);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for (int i = 0; i < node->mNumMeshes; i++)
    {
        auto &obj = m_Entries[node->mMeshes[i]];

        glBindBuffer(GL_ARRAY_BUFFER, obj.first.VB);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, pos_));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, tex_));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, norm_));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.first.IB);

        auto dc = obj.second->diffuse_factor_;

        glUniformMatrix4fv(context.mvp_id_, 1, GL_FALSE, &context.vp[0][0]);
        glUniform3f(context.diffuse_id_, dc.x, dc.y, dc.z);

        if (auto &texture = obj.second)
        {
            texture->Bind(GL_TEXTURE0);
        }
        else
        {
            log_.Warning() << "Texture missing!";
        }

        glDrawElements(GL_TRIANGLES, obj.first.indices_.size(), GL_UNSIGNED_INT, 0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    for (int i = 0; i < node->mNumChildren; i++)
    {
        RenderByOpenGL(context, node->mChildren[i]);
    }
}

boost::optional<Intersection> Mesh::Raytrace(const glm::vec3 &source,
                                             const glm::vec3 &target,
                                             const OpenGLRenderingContext &context,
                                             int recursion_depth)
{
    boost::optional<Intersection> intersection_so_far;
    auto intersection_dist_so_far = std::numeric_limits<float>::infinity();

    glm::vec3 pos, normal;
    glm::vec3 barycentric;
    float intersection_dist;

    for (const auto &obj : m_Entries)
    {
        const auto &vertices = obj.first.vertices_;
        const auto &indices = obj.first.indices_;

        for (int i = 0; i < indices.size(); i += 3)
        {
            auto vertex1 = vertices[indices[i + 0]];
            auto vertex2 = vertices[indices[i + 1]];
            auto vertex3 = vertices[indices[i + 2]];

            if (auto intersection = RayIntersectsTriangle(source, target, vertex1.pos_,
                                                          vertex2.pos_, vertex3.pos_))
            {
                std::tie(intersection_dist, pos, barycentric, normal) = *intersection;

                if (recursion_depth == 0)
                {
                    if (intersection && intersection_dist < 1.0f)
                        return Intersection();
                    else
                        continue;
                }

                if (intersection_dist < intersection_dist_so_far)
                {
                    Intersection in;
                    in.diffuse = context.ambient;

                    for (const auto &light : *context.lights_)
                    {

                        if (Raytrace(pos, light.position - pos, context,
                                     recursion_depth - 1))
                            continue;

                        auto light_to_intersection = glm::normalize(light.position - pos);
                        auto angle_factor =
                            glm::abs(glm::dot<3, float, glm::qualifier::highp>(
                                light_to_intersection, glm::normalize(normal)));
                        in.diffuse += angle_factor * light.intensity_rgb;
                    }

                    in.diffuse *=
                        GetDiffuse(vertex1, vertex2, vertex3, barycentric, *obj.second);

                    intersection_so_far = in;
                    intersection_dist_so_far = intersection_dist;
                }
            }
        }
    }

    return intersection_so_far;
}
