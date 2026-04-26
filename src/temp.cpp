// sceneShader.Use();
//
// sceneShader.SetMat4("model", model);
// sceneShader.SetMat4("view", camera.viewMatrix);
// sceneShader.SetMat4("projection", camera.projectionMatrix);
// sceneShader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
//
// sceneShader.SetVec3("viewPosition", camera.position);
// sceneShader.SetFloat("material.shininess", 32.0f);
//
// sceneShader.SetVec3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
// sceneShader.SetVec3("directionalLight.ambient", glm::vec3(0.3f, 0.3f, 0.3f));
// sceneShader.SetVec3("directionalLight.diffuse", glm::vec3(0.7f, 0.7f, 0.7f));
// sceneShader.SetVec3("directionalLight.specular", glm::vec3(0.8f, 0.8f, 0.8f));
//
// snowTerrain.Draw(sceneShader);
