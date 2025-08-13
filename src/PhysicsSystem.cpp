#include "PhysicsSystem.h"
#include <glm/gtc/type_ptr.hpp> // Include for glm::make_mat4


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

void PhysicsSystem::Update(float deltaTime) {
    // Step the physics simulation
    // You can adjust the parameters for accuracy and performance
    dynamicsWorld->stepSimulation(deltaTime, 10); // 10 is the maximum number of internal substeps
}

btRigidBody* PhysicsSystem::CreateGroundPlane(const glm::vec3& position) {
    // Create a static ground plane
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0); // Normal and distance from origin
    collisionShapes.push_back(groundShape);

    btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(position.x, position.y, position.z)));
    motionStates.push_back(groundMotionState);

    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0)); // Mass 0 for static object
    btRigidBody* groundBody = new btRigidBody(groundRigidBodyCI);

    dynamicsWorld->addRigidBody(groundBody);

    return groundBody;
}

btRigidBody* PhysicsSystem::CreateCube(const glm::vec3& position, float mass) {
    // Create a dynamic cube
    btCollisionShape* cubeShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)); // Half extents
    collisionShapes.push_back(cubeShape);

    btDefaultMotionState* cubeMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(position.x, position.y, position.z)));
    motionStates.push_back(cubeMotionState);

    btVector3 fallInertia(0, 0, 0);
    cubeShape->calculateLocalInertia(mass, fallInertia);

    btRigidBody::btRigidBodyConstructionInfo cubeRigidBodyCI(mass, cubeMotionState, cubeShape, fallInertia);
    btRigidBody* cubeBody = new btRigidBody(cubeRigidBodyCI);

    dynamicsWorld->addRigidBody(cubeBody);

    return cubeBody;
}

btRigidBody* PhysicsSystem::CreateSphere(const glm::vec3& position, float radius, float mass) {
    // Create a dynamic sphere
    btCollisionShape* sphereShape = new btSphereShape(radius);
    collisionShapes.push_back(sphereShape);

    btDefaultMotionState* sphereMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(position.x, position.y, position.z)));
    motionStates.push_back(sphereMotionState);

    btVector3 fallInertia(0, 0, 0);
    sphereShape->calculateLocalInertia(mass, fallInertia);

    btRigidBody::btRigidBodyConstructionInfo sphereRigidBodyCI(mass, sphereMotionState, sphereShape, fallInertia);
    btRigidBody* sphereBody = new btRigidBody(sphereRigidBodyCI);

    dynamicsWorld->addRigidBody(sphereBody);

    return sphereBody;
}

void PhysicsSystem::ApplyForce(btRigidBody* body, const glm::vec3& force) {
    if (body) {
        body->activate(true); // Activate the body if it was sleeping
        body->applyCentralForce(btVector3(force.x, force.y, force.z));
    }
}
