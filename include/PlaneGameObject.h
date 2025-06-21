
#ifndef MISTENGINE_PLANEGAMEOBJECT_H
#define MISTENGINE_PLANEGAMEOBJECT_H

#include "GameObject.h"
#include "Shader.h"

class PlaneGameObject : public GameObject {
public:
    // Constructor
    PlaneGameObject(unsigned int vao);

    // Destructor
    ~PlaneGameObject() override {}

    // Render the plane
    void render() override;

private:
    unsigned int m_vao;
};

#endif // MISTENGINE_PLANEGAMEOBJECT_H