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

    [[nodiscard]] std::vector<std::vector<glm::vec3>> GetTriangles(const glm::mat4& modelMatrix) const;

    void loadModel();
    void Draw(const Shader &shader) const;
private:
    std::vector<Mesh> meshes;
    std::vector<Texture> loadedTextures;

    std::string modelPath;
    std::string directory;

    std::vector<Texture> loadMaterialTextures(const aiMaterial *material, aiTextureType type, const std::string &typeName);

    void processMesh(aiMesh *mesh, const aiScene *scene);
    void processNode(const aiNode *node, const aiScene *scene);
};


#endif