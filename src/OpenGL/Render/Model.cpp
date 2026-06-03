#include "Model.h"

#include <glm/vec4.hpp>
#include "Exceptions/Error.h"
#include "Exceptions/MPMException.h"
#include "../Shaders/TextureLoader.h"

Model::Model(const std::string &path)
{
    this->modelPath = std::string(ASSETS_PATH) + path;
}

std::vector<std::vector<glm::vec3>> Model::GetTriangles(const glm::mat4& modelMatrix) const
{
    std::vector<std::vector<glm::vec3>> triangles;

    for (const auto& mesh : this->meshes)
    {
        for (auto& triangle : mesh.GetTriangles())
        {
            const glm::vec3 firstVertex = glm::vec3(modelMatrix * glm::vec4(triangle[0], 1.0f));
            const glm::vec3 secondVertex = glm::vec3(modelMatrix * glm::vec4(triangle[1], 1.0f));
            const glm::vec3 thirdVertex = glm::vec3(modelMatrix * glm::vec4(triangle[2], 1.0f));

            triangles.push_back({firstVertex, secondVertex, thirdVertex});
        }
    }

    return triangles;
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

void Model::Draw(const Shader &shader) const
{
    for (auto &mesh: this->meshes)
    {
        mesh.Draw(shader);
    }
}

std::vector<Texture> Model::loadMaterialTextures(const aiMaterial *material, const aiTextureType type, const std::string &typeName)
{
    std::vector<Texture> textures;

    for (int textureIndex = 0; textureIndex < material->GetTextureCount(type); textureIndex++)
    {
        aiString localPath;

        material->GetTexture(type, textureIndex, &localPath);

        bool skip = false;

        for (auto & loadedTexture : this->loadedTextures)
        {
            if (std::strcmp(loadedTexture.path.data(), localPath.C_Str()) == 0)
            {
                textures.push_back(loadedTexture);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            Texture texture;

            texture.id = TextureLoader::Load(localPath.C_Str(), this->directory);
            texture.type = typeName;
            texture.path = localPath.C_Str();
            textures.push_back(texture);

            this->loadedTextures.push_back(texture);
        }
    }

    return textures;
}

void Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (int vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++)
    {
        Vertex vertex{};
        glm::vec3 informationBuffer;

        informationBuffer.x = mesh->mVertices[vertexIndex].x;
        informationBuffer.y = mesh->mVertices[vertexIndex].y;
        informationBuffer.z = mesh->mVertices[vertexIndex].z;

        vertex.Position = informationBuffer;

        informationBuffer.x = mesh->mNormals[vertexIndex].x;
        informationBuffer.y = mesh->mNormals[vertexIndex].y;
        informationBuffer.z = mesh->mNormals[vertexIndex].z;

        vertex.Normal = informationBuffer;

        if (mesh->mTextureCoords[0])
        {
            glm::vec2 textureCoordinates;

            textureCoordinates.x = mesh->mTextureCoords[0][vertexIndex].x;
            textureCoordinates.y = mesh->mTextureCoords[0][vertexIndex].y;

            vertex.TextureCoordinates = textureCoordinates;
        }
        else
        {
            vertex.TextureCoordinates = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (int faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
    {
        aiFace face = mesh->mFaces[faceIndex];

        for (int currentIndex = 0; currentIndex < face.mNumIndices; currentIndex++)
        {
            indices.push_back(face.mIndices[currentIndex]);
        }
    }

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    this->meshes.emplace_back(vertices, indices, textures);
}

void Model::processNode(const aiNode *node, const aiScene *scene)
{
    try
    {
        this->meshes.reserve(this->meshes.size() + node->mNumMeshes);
    }
    catch (const std::exception& exception)
    {
        throw MPMException(exception.what(), Error::ModelMemoryAllocation);
    }

    for (int meshIndex = 0; meshIndex < node->mNumMeshes; meshIndex++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[meshIndex]];
        this->processMesh(mesh, scene);
    }

    for (int childrenIndex = 0; childrenIndex < node->mNumChildren; childrenIndex++)
    {
        this->processNode(node->mChildren[childrenIndex], scene);
    }
}