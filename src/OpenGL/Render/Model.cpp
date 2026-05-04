#include "Model.h"

#include <array>

#include <glm/vec4.hpp>

#include "Exceptions/Error.h"
#include "Exceptions/MPMException.h"
#include "../Shaders/TextureLoader.h"

Model::Model(const std::string &path)
{
    this->modelPath = path;
}

void Model::Draw(const Shader &shader) const
{
    for (auto &mesh : this->meshes)
    {
        mesh.Draw(shader);
    }
}

void Model::loadModel()
{
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(this->modelPath, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        throw MPMException("Failed to load model", Error::ModelLoad);
    }

    this->directory = this->modelPath.substr(0, this->modelPath.find_last_of('/'));

    this->processNode(scene->mRootNode, scene);
}

void Model::processNode(const aiNode *node, const aiScene *scene)
{
    this->meshes.reserve(this->meshes.size() + node->mNumMeshes);

    for (int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        this->processMesh(mesh, scene);
    }

    for (int i = 0; i < node->mNumChildren; i++)
    {
        this->processNode(node->mChildren[i], scene);
    }
}

void Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex{};
        glm::vec3 vector;

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;

        vertex.Position = vector;

        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;

        vertex.Normal = vector;

        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;

            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;

            vertex.TextureCoordinates = vec;
        }
        else
        {
            vertex.TextureCoordinates = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    this->meshes.emplace_back(vertices, indices, textures);
}

std::vector<std::array<glm::vec3, 3>> Model::GetTriangles(const glm::mat4& modelMatrix) const
{
    std::vector<std::array<glm::vec3, 3>> triangles;

    for (const auto& mesh : this->meshes)
    {
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            glm::vec3 v0 = glm::vec3(modelMatrix * glm::vec4(mesh.vertices[mesh.indices[i    ]].Position, 1.0f));
            glm::vec3 v1 = glm::vec3(modelMatrix * glm::vec4(mesh.vertices[mesh.indices[i + 1]].Position, 1.0f));
            glm::vec3 v2 = glm::vec3(modelMatrix * glm::vec4(mesh.vertices[mesh.indices[i + 2]].Position, 1.0f));

            triangles.push_back({v0, v1, v2});
        }
    }

    return triangles;
}

std::vector<Texture> Model::loadMaterialTextures(const aiMaterial *material, const aiTextureType type, const std::string &typeName)
{
    std::vector<Texture> textures;

    for (int textureIndex = 0; textureIndex < material->GetTextureCount(type); textureIndex++)
    {
        aiString localPath;

        material->GetTexture(type, textureIndex, &localPath);

        bool skip = false;

        for (int loadedTextureIndex = 0; loadedTextureIndex < this->loadedTextures.size(); loadedTextureIndex++)
        {
            if (std::strcmp(loadedTextures[loadedTextureIndex].path.data(), localPath.C_Str()) == 0)
            {
                textures.push_back(loadedTextures[loadedTextureIndex]);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            Texture texture;

            texture.id = TextureLoader::StaticLoad(localPath.C_Str(), this->directory);
            texture.type = typeName;
            texture.path = localPath.C_Str();
            textures.push_back(texture);

            this->loadedTextures.push_back(texture);
        }
    }

    return textures;
}
