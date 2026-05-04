#ifndef MPM_METHOD_MODEL_H
#define MPM_METHOD_MODEL_H

#include <array>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/mat4x4.hpp>

#include "Mesh.h"
#include "OpenGL/Shaders/Shader.h"


class Model
{
public:
    explicit Model(const std::string &path);

    void loadModel();
    void Draw(const Shader &shader) const;
    std::vector<std::array<glm::vec3, 3>> GetTriangles(const glm::mat4& modelMatrix) const;
private:
    std::vector<Mesh> meshes;
    std::vector<Texture> loadedTextures;

    std::string modelPath;
    std::string directory;

    void processNode(const aiNode *node, const aiScene *scene);

    void processMesh(aiMesh *mesh, const aiScene *scene);

    std::vector<Texture> loadMaterialTextures(const aiMaterial *material, aiTextureType type, const std::string &typeName);
};


#endif