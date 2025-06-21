
#ifndef MISTENGINE_BACKPACKGAMEOBJECT_H
#define MISTENGINE_BACKPACKGAMEOBJECT_H

#include "GameObject.h"
#include "Model.h"
#include "Shader.h"

class BackpackGameObject : public GameObject {
public:
    // Constructor
    BackpackGameObject(const std::string& modelPath);

    // Destructor
    ~BackpackGameObject() override {}

    // Render the backpack model
    void render() override;

private:
    Model m_model; // The backpack model
};

#endif // MISTENGINE_BACKPACKGAMEOBJECT_H