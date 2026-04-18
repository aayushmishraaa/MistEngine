#include "PhysicsSystem.h"

#include <glm/gtc/type_ptr.hpp>

using Mist::Physics::ScopedCollisionShape;
using Mist::Physics::ScopedMotionState;
using Mist::Physics::ScopedRigidBody;

PhysicsSystem::PhysicsSystem()
    : m_CollisionConfiguration(std::make_unique<btDefaultCollisionConfiguration>())
    , m_Dispatcher(std::make_unique<btCollisionDispatcher>(m_CollisionConfiguration.get()))
    , m_Broadphase(std::make_unique<btDbvtBroadphase>())
    , m_Solver(std::make_unique<btSequentialImpulseConstraintSolver>())
    , m_DynamicsWorld(std::make_unique<btDiscreteDynamicsWorld>(
          m_Dispatcher.get(), m_Broadphase.get(), m_Solver.get(), m_CollisionConfiguration.get())) {
    m_DynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

PhysicsSystem::~PhysicsSystem() {
    // Order matters: bodies must leave the world before the world dies.
    // Clearing m_Bodies first triggers each ScopedRigidBody's dtor, which
    // removes itself from m_DynamicsWorld (still alive at this point).
    m_Bodies.clear();
    m_Shapes.clear();
    // m_DynamicsWorld, m_Solver, m_Broadphase, m_Dispatcher,
    // m_CollisionConfiguration are destroyed in reverse declaration order,
    // which is exactly what Bullet requires.
}

void PhysicsSystem::Update(float deltaTime) {
    m_DynamicsWorld->stepSimulation(deltaTime, 10);
}

btRigidBody* PhysicsSystem::addBody(ScopedCollisionShape shape,
                                     const glm::vec3& position,
                                     float mass) {
    ScopedMotionState motionState(new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(position.x, position.y, position.z))));

    btVector3 inertia(0, 0, 0);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, inertia);
    }

    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState.get(), shape.get(), inertia);
    auto body = std::make_unique<btRigidBody>(info);
    btRigidBody* raw = body.get();

    m_DynamicsWorld->addRigidBody(raw);

    // Shape is kept alive in m_Shapes because Bullet stores a non-owning
    // pointer to the shape inside the body's construction info.
    m_Shapes.push_back(std::move(shape));
    m_Bodies.emplace_back(m_DynamicsWorld.get(), std::move(body), std::move(motionState));
    return raw;
}

btRigidBody* PhysicsSystem::CreateGroundPlane(const glm::vec3& position) {
    ScopedCollisionShape shape(new btStaticPlaneShape(btVector3(0, 1, 0), 0));
    return addBody(std::move(shape), position, 0.0f);
}

btRigidBody* PhysicsSystem::CreateCube(const glm::vec3& position, float mass) {
    ScopedCollisionShape shape(new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)));
    return addBody(std::move(shape), position, mass);
}

btRigidBody* PhysicsSystem::CreateSphere(const glm::vec3& position, float radius, float mass) {
    ScopedCollisionShape shape(new btSphereShape(radius));
    return addBody(std::move(shape), position, mass);
}

void PhysicsSystem::ApplyForce(btRigidBody* body, const glm::vec3& force) {
    if (body) {
        body->activate(true);
        body->applyCentralForce(btVector3(force.x, force.y, force.z));
    }
}
