#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <vector>
#include "Model.h"
#include "Shader.h"

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    void AddModel(Model* model);
    void RenderScene(Shader& shader);

private:
    std::vector<Model*> models;
};

#endif // SCENEMANAGER_H