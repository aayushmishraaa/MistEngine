#include "Systems.h"
#include "Scene.h"
#include "Renderer.h"
#include "Components.h"
#include "Mesh.h" // For RenderComponent::mesh->Draw

#include <glm/gtc/type_ptr.hpp> // For glm::make_mat4, glm::value_ptr
#include <glm/gtc/matrix_transform.hpp> // For glm::translate, glm::scale
#include <glm/gtc/quaternion.hpp> // For glm::quat_cast

#include <iostream>

// Implementations for RenderSystem
void RenderSystem::update(float deltaTime, Scene& scene) {
    if (!renderer) {
        std::cerr << "RenderSystem requires a valid renderer pointer." << std::endl;
        return;
    }

    // *** Shadow Map Pass ***
    renderer->glViewport(0, 0, renderer->shadowWidth, renderer->shadowHeight);
    renderer->glBindFramebuffer(GL_FRAMEBUFFER, renderer->depthMapFBO);
    renderer->glClear(GL_DEPTH_BUFFER_BIT);

    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(-renderer->lightDir * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    renderer->depthShader.use();
    renderer->depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Render entities with PositionComponent and RenderComponent to depth map
    auto renderableEntities = scene.ecsManager.getEntitiesWith<PositionComponent, RenderComponent>();
    for (Entity entity : renderableEntities) {
        PositionComponent* posComp = scene.ecsManager.getComponent<PositionComponent>(entity);
        RenderComponent* renderComp = scene.ecsManager.getComponent<RenderComponent>(entity);

        if (posComp && renderComp && renderComp->mesh) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posComp->position);
            model *= glm::mat4_cast(posComp->rotation);
            model = glm::scale(model, posComp->scale);

            renderer->depthShader.setMat4("model", model);
            renderComp->mesh->Draw(renderer->depthShader);
        }
    }

    renderer->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // *** Main Render Pass ***
    renderer->glViewport(0, 0, renderer->screenWidth, renderer->screenHeight);
    renderer->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(renderer->camera.Zoom), (float)renderer->screenWidth / (float)renderer->screenHeight, 0.1f, 100.0f);
    glm::mat4 view = renderer->camera.GetViewMatrix();

    // Render scene with shadows (entities with PositionComponent and RenderComponent)
    renderer->objectShader.use();
    renderer->objectShader.setMat4("projection", projection);
    renderer->objectShader.setMat4("view", view);
    renderer->objectShader.setVec3("lightDir", renderer->lightDir);
    renderer->objectShader.setVec3("lightColor", renderer->lightColor);
    renderer->objectShader.setVec3("viewPos", renderer->camera.Position);
    renderer->objectShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    renderer->glActiveTexture(GL_TEXTURE0);
    renderer->glBindTexture(GL_TEXTURE_2D, renderer->depthMap);
    renderer->objectShader.setInt("shadowMap", 0);

    for (Entity entity : renderableEntities) {
        PositionComponent* posComp = scene.ecsManager.getComponent<PositionComponent>(entity);
        RenderComponent* renderComp = scene.ecsManager.getComponent<RenderComponent>(entity);

        if (posComp && renderComp && renderComp->mesh) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posComp->position);
            model *= glm::mat4_cast(posComp->rotation);
            model = glm::scale(model, posComp->scale);

            renderer->objectShader.setMat4("model", model);
            renderComp->mesh->Draw(renderer->objectShader);
        }
    }

    // TODO: Integrate glowing orb rendering into the ECS (e.g., with an OrbComponent and/or a dedicated rendering system)

    // Swap buffers is done in the main loop after all systems update
}

// Implementations for PhysicsSystem
PhysicsSystem::PhysicsSystem() {
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    overlappingPairCache = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

    // Set gravity (you can adjust this)
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

PhysicsSystem::~PhysicsSystem() {
    // Clean up physics objects in reverse order of creation
    // Remove rigid bodies from the world and delete them
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }

    // Delete collision shapes
    for (btCollisionShape* shape : collisionShapes) {
        delete shape;
    }
    collisionShapes.clear();

    // Delete motion states (already deleted with rigid bodies above, but clear the vector)
    motionStates.clear();

    // Delete dynamics world and related components
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
}

void PhysicsSystem::update(float deltaTime, Scene& scene) {
    // Step the physics simulation
    dynamicsWorld->stepSimulation(deltaTime, 10); // 10 is the maximum number of internal substeps

    // Update the position and rotation of entities based on physics simulation
    auto physicsEntities = scene.ecsManager.getEntitiesWith<PositionComponent, PhysicsComponent>();
    for (Entity entity : physicsEntities) {
        PositionComponent* posComp = scene.ecsManager.getComponent<PositionComponent>(entity);
        PhysicsComponent* physicsComp = scene.ecsManager.getComponent<PhysicsComponent>(entity);

        if (posComp && physicsComp && physicsComp->rigidBody) {
            btTransform transform = physicsComp->rigidBody->getWorldTransform();

            // Update position
            posComp->position = glm::vec3(transform.getOrigin().getX(), transform.getOrigin().getY(), transform.getOrigin().getZ());

            // Update rotation
            btQuaternion rotation = transform.getRotation();
            posComp->rotation = glm::quat(rotation.getW(), rotation.getX(), rotation.getY(), rotation.getZ());
        }
    }
}

// Helper to convert btTransform to glm::mat4
void updateModelMatrixFromPhysics(btRigidBody* body, glm::mat4& modelMatrix) {
    if (body) {
        const btTransform& transform = body->getCenterOfMassTransform();
        float matrix[16];
        transform.getOpenGLMatrix(matrix);
        modelMatrix = glm::make_mat4(matrix);
    }
}