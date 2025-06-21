#include "SceneManager.h"

SceneManager::SceneManager() {}

SceneManager::~SceneManager() {}

void SceneManager::AddModel(Model* model) {
    models.push_back(model);
}

void SceneManager::RenderScene(Shader& shader) {
    for (Model* model : models) {
        model->Draw(shader);
    }
}