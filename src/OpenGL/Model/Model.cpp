#include "Model.h"

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
    for (int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        this->meshes.push_back(this->processMesh(mesh, scene));
    }

    for (int i = 0; i < node->mNumChildren; i++)
    {
        this->processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
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

    return {vertices, indices, textures};
}

std::vector<Texture> Model::loadMaterialTextures(const aiMaterial *mat, const aiTextureType type, const std::string &typeName)
{
    std::vector<Texture> textures;

    for (int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString localPath;

        mat->GetTexture(type, i, &localPath);

        bool skip = false;

        for (int j = 0; j < this->loadedTextures.size(); j++)
        {
            if (std::strcmp(loadedTextures[j].path.data(), localPath.C_Str()) == 0)
            {
                textures.push_back(loadedTextures[j]);
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
