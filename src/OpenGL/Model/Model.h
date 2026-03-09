#ifndef MPM_METHOD_MODEL_H
#define MPM_METHOD_MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "OpenGL/Shaders/Shader.h"


class Model
{
public:
    explicit Model(const std::string &path);

    void loadModel();
    void Draw(const Shader &shader) const;
private:
    std::vector<Mesh> meshes;
    std::vector<Texture> loadedTextures;

    std::string modelPath;
    std::string directory;

    void processNode(const aiNode *node, const aiScene *scene);

    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    std::vector<Texture> loadMaterialTextures(const aiMaterial *mat, aiTextureType type, const std::string &typeName);
};


#endif