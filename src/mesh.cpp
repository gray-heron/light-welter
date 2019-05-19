
#include "mesh.h"
#include "config.h"
#include "exceptions.h"

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

Mesh::MeshEntry::MeshEntry(std::vector<Vertex> &&Vertices,
                           std::vector<glm::u32> &&Indices, Material &mat)
    : vertices_(std::move(Vertices)), indices_(std::move(Indices)), material_(mat){};

void Mesh::MeshEntry::SetupForOpenGL()
{
    glGenBuffers(1, &VB);
    glBindBuffer(GL_ARRAY_BUFFER, VB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices_.size(), &vertices_[0],
                 GL_STATIC_DRAW);

    glGenBuffers(1, &IB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices_.size(),
                 &indices_[0], GL_STATIC_DRAW);

    material_.SetupForOpenGL();
}

Mesh::MeshEntry::~MeshEntry()
{
    // APPLY RULE OF FIVE
    // FIXME
    /*
    glDeleteBuffers(1, &VB);
    glDeleteBuffers(1, &IB);
    */
}

Mesh::Mesh(std::string filename, Scene &scene)
    : lower_bound_(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max()),
      upper_bound_(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(),
                   std::numeric_limits<float>::min())
{
    const aiScene *ai_scene = importer_.ReadFile(
        filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                              aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

    STRONG_ASSERT(ai_scene,
                  "Error parsing " + filename + " : " + importer_.GetErrorString());

    std::string::size_type slash = filename.find_last_of("/");
    std::string dir;

    log_.Info() << "Scene has textures? " << ai_scene->HasTextures();
    log_.Info() << "Scene has lights? " << ai_scene->HasLights();

    if (slash == std::string::npos)
        dir = ".";
    else if (slash == 0)
        dir = "/";
    else
        dir = filename.substr(0, slash);

    for (unsigned int i = 0; i < ai_scene->mNumMaterials; i++)
        materials_.emplace_back(ai_scene->mMaterials[i], dir);

    for (unsigned int i = 0; i < ai_scene->mNumMeshes; i++)
    {
        auto &material = materials_[ai_scene->mMeshes[i]->mMaterialIndex];
        submeshes_.emplace_back(
            InitMesh(ai_scene->mMeshes[i], scene.area_lights_, material));
    }

    scene_ = ai_scene;
}

Mesh::~Mesh() {}

void Mesh::SetupForOpenGL()
{
    for (auto &submesh : submeshes_)
    {
        submesh.SetupForOpenGL();
    }
}

Mesh::MeshEntry Mesh::InitMesh(const aiMesh *mesh, std::vector<AreaLight> &lights,
                               const Material &material)
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

        for (int i = 0; i < 3; i++)
        {
            if (v.pos_[i] < lower_bound_[i])
                lower_bound_[i] = v.pos_[i];
            else if (v.pos_[i] > upper_bound_[i])
                upper_bound_[i] = v.pos_[i];
        }

        Vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace &Face = mesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        Indices.push_back(Face.mIndices[0]);
        Indices.push_back(Face.mIndices[1]);
        Indices.push_back(Face.mIndices[2]);

        if (material.IsEmissive())
        {
            lights.emplace_back(Vertices[Face.mIndices[0]].pos_,
                                Vertices[Face.mIndices[1]].pos_,
                                Vertices[Face.mIndices[2]].pos_, material);
        }
    }

    return MeshEntry(std::move(Vertices), std::move(Indices),
                     materials_[mesh->mMaterialIndex]);
}

void Mesh::RenderByOpenGL(OpenGLRenderingContext context, aiNode *node)
{
    if (node == nullptr)
        node = scene_->mRootNode;

    context.vp = context.vp * aiMatrix4x4ToGlm(node->mTransformation);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        auto &obj = submeshes_[node->mMeshes[i]];

        glBindBuffer(GL_ARRAY_BUFFER, obj.VB);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, pos_));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, tex_));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const GLvoid *)offsetof(Vertex, norm_));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.IB);

        glUniformMatrix4fv(context.mvp_id_, 1, GL_FALSE, &context.vp[0][0]);

        obj.material_.BindForOpenGL(context, GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, obj.indices_.size(), GL_UNSIGNED_INT, 0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        RenderByOpenGL(context, node->mChildren[i]);
    }
}

Material &Mesh::GetMaterial(uint32_t obj_index_)
{
    return submeshes_[obj_index_].material_;
}

glm::vec3 Mesh::GetUpperBound() { return upper_bound_; }
glm::vec3 Mesh::GetLowerBound() { return lower_bound_; }