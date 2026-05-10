#ifndef MPM_METHOD_MODEL_H
#define MPM_METHOD_MODEL_H

#include <array>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/mat4x4.hpp>
#include "Mesh.h"
#include "OpenGL/Shaders/Shader.h"

class Model
{
public:
    explicit Model(const std::string &path);

    void loadModel();
    void Draw(const Shader &shader) const;

    [[nodiscard]] std::vector<std::array<glm::vec3, 3>> GetTriangles(const glm::mat4& modelMatrix) const;
private:
    std::string modelPath;
    std::string directory;

    std::vector<Mesh> meshes;
    std::vector<Texture> loadedTextures;

    void processMesh(aiMesh *mesh, const aiScene *scene);
    void processNode(const aiNode *node, const aiScene *scene);

    std::vector<Texture> loadMaterialTextures(const aiMaterial *material, aiTextureType type, const std::string &typeName);
};


#endif