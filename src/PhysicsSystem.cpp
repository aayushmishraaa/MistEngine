#include "PhysicsSystem.h"
#include <glm/gtc/type_ptr.hpp>

PhysicsSystem::PhysicsSystem() {}

PhysicsSystem::~PhysicsSystem() {
    Cleanup();
}

void PhysicsSystem::Initialize() {
    // Create the physics world
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    overlappingPairCache = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    
    // Set gravity
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

void PhysicsSystem::Update(float deltaTime) {
    dynamicsWorld->stepSimulation(deltaTime, 10);
}

btRigidBody* PhysicsSystem::CreateRigidBody(float mass, const glm::vec3& position, const glm::vec3& size) {
    btCollisionShape* shape = new btBoxShape(btVector3(size.x/2, size.y/2, size.z/2));
    collisionShapes.push_back(shape);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));

    btScalar bodyMass = mass;
    btVector3 localInertia(0, 0, 0);
    if (bodyMass != 0.0f)
        shape->calculateLocalInertia(bodyMass, localInertia);

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(bodyMass, motionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);

    dynamicsWorld->addRigidBody(body);
    rigidBodies.push_back(body);

    return body;
}
void PhysicsSystem::ApplyForce(btRigidBody* body, const glm::vec3& force) {
    if (body) {
        body->applyCentralForce(btVector3(force.x, force.y, force.z));
    }
}

glm::mat4 PhysicsSystem::BulletToGLM(const btTransform& transform) {
    btScalar matrix[16];
    transform.getOpenGLMatrix(matrix);
    return glm::make_mat4(matrix);

}

btTransform PhysicsSystem::GLMToBullet(const glm::mat4& matrix) {
    btTransform transform;
    transform.setFromOpenGLMatrix(&matrix[0][0]);
    return transform;
}

void PhysicsSystem::Cleanup() {
    for (int i = rigidBodies.size() - 1; i >= 0; i--) {
        btRigidBody* body = rigidBodies[i];
        dynamicsWorld->removeRigidBody(body);
        delete body->getMotionState();
        delete body;
    }
    rigidBodies.clear();

    for (int i = 0; i < collisionShapes.size(); i++) {
        delete collisionShapes[i];
    }
    collisionShapes.clear();

    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
}