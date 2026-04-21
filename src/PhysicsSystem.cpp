#include "PhysicsSystem.h"

#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <functional>

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
    // Bodies must leave the world before the world dies. Clearing m_Bodies
    // first triggers each ScopedRigidBody's dtor, which removes itself from
    // m_DynamicsWorld (still alive). Shapes follow; then world + infra die
    // in reverse declaration order — exactly what Bullet requires.
    m_Bodies.clear();
    m_Shapes.clear();
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

btCollisionShape* PhysicsSystem::CreateShape(CollisionShape s,
                                             const glm::vec3& halfExtents,
                                             float radius,
                                             float height) {
    switch (s) {
        case CollisionShape::Box:
            return new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
        case CollisionShape::Sphere:
            return new btSphereShape(radius);
        case CollisionShape::Capsule:
            // Bullet's capsule is (radius, cylinder-height) — end caps are
            // added on top. Total vertical extent = height + 2*radius.
            return new btCapsuleShape(radius, height);
        case CollisionShape::StaticPlane:
            return new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    }
    return nullptr;
}

btRigidBody* PhysicsSystem::CreateBodyFromComponent(const glm::vec3& position,
                                                   const PhysicsComponent& pc) {
    ScopedCollisionShape shape(CreateShape(pc.shape, pc.halfExtents, pc.radius, pc.height));
    btRigidBody* raw = addBody(std::move(shape), position, pc.mass);
    ApplyMaterial(raw, pc);
    return raw;
}

void PhysicsSystem::DestroyBody(btRigidBody* body) {
    if (!body) return;
    for (std::size_t i = 0; i < m_Bodies.size(); ++i) {
        if (m_Bodies[i].get() == body) {
            // Swap-and-pop. ScopedRigidBody dtor removes it from the
            // world; the paired shape can then be freed safely.
            std::swap(m_Bodies[i], m_Bodies.back());
            std::swap(m_Shapes[i], m_Shapes.back());
            m_Bodies.pop_back();
            m_Shapes.pop_back();
            return;
        }
    }
}

void PhysicsSystem::ApplyMaterial(btRigidBody* body, const PhysicsComponent& pc) {
    if (!body) return;
    body->setFriction(pc.friction);
    body->setRestitution(pc.restitution);
    body->setDamping(pc.linearDamping, pc.angularDamping);

    int flags = body->getCollisionFlags();
    if (pc.kinematic) {
        flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
        body->setActivationState(DISABLE_DEACTIVATION);
    } else {
        flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
    }
    body->setCollisionFlags(flags);
}

std::uint64_t PhysicsSystem::ComputeShapeHash(const PhysicsComponent& pc) {
    // FNV-1a over the bit patterns of the geometry params. Only fields
    // that require a shape rebuild participate — material params are
    // applied live, so they're deliberately excluded.
    std::uint64_t h = 1469598103934665603ull;
    auto mix = [&](const void* data, std::size_t n) {
        const auto* p = static_cast<const unsigned char*>(data);
        for (std::size_t i = 0; i < n; ++i) {
            h ^= p[i];
            h *= 1099511628211ull;
        }
    };
    auto kind = static_cast<std::uint8_t>(pc.shape);
    mix(&kind, sizeof(kind));
    mix(&pc.halfExtents, sizeof(pc.halfExtents));
    mix(&pc.radius, sizeof(pc.radius));
    mix(&pc.height, sizeof(pc.height));
    mix(&pc.mass, sizeof(pc.mass));
    return h;
}

void PhysicsSystem::EnsureBody(const glm::vec3& position, PhysicsComponent& pc) {
    const std::uint64_t want = ComputeShapeHash(pc);

    const bool needsRebuild = (pc.rigidBody == nullptr) || (pc.shapeHash != want);
    if (needsRebuild) {
        if (pc.rigidBody) {
            DestroyBody(pc.rigidBody);
            pc.rigidBody = nullptr;
        }
        pc.rigidBody  = CreateBodyFromComponent(position, pc);
        pc.shapeHash  = want;
    } else {
        // Geometry unchanged — just push material updates through.
        ApplyMaterial(pc.rigidBody, pc);
    }
}

PhysicsSystem::RayHit
PhysicsSystem::Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist) const {
    RayHit out;
    glm::vec3 n = glm::normalize(dir);
    if (maxDist <= 0.0f || glm::dot(n, n) <= 0.0f) return out;

    btVector3 from(origin.x, origin.y, origin.z);
    btVector3 to = from + btVector3(n.x, n.y, n.z) * maxDist;

    btCollisionWorld::ClosestRayResultCallback cb(from, to);
    m_DynamicsWorld->rayTest(from, to, cb);
    if (!cb.hasHit()) return out;

    out.hit      = true;
    out.point    = {cb.m_hitPointWorld.x(),  cb.m_hitPointWorld.y(),  cb.m_hitPointWorld.z()};
    out.normal   = {cb.m_hitNormalWorld.x(), cb.m_hitNormalWorld.y(), cb.m_hitNormalWorld.z()};
    out.fraction = cb.m_closestHitFraction;

    // User-pointer encodes the ECS Entity id (set by EnsureBody via
    // setUserIndex). Unpack via reinterpret-cast of the int index —
    // entity ids fit in 32 bits, so we round-trip losslessly.
    if (cb.m_collisionObject) {
        int idx = cb.m_collisionObject->getUserIndex();
        if (idx >= 0) out.entity = static_cast<std::uint32_t>(idx);
    }
    return out;
}

btRigidBody* PhysicsSystem::CreateGroundPlane(const glm::vec3& position) {
    ScopedCollisionShape shape(CreateShape(CollisionShape::StaticPlane, {}, 0.0f, 0.0f));
    return addBody(std::move(shape), position, 0.0f);
}

btRigidBody* PhysicsSystem::CreateCube(const glm::vec3& position, float mass) {
    ScopedCollisionShape shape(CreateShape(CollisionShape::Box, glm::vec3(0.5f), 0.0f, 0.0f));
    return addBody(std::move(shape), position, mass);
}

btRigidBody* PhysicsSystem::CreateSphere(const glm::vec3& position, float radius, float mass) {
    ScopedCollisionShape shape(CreateShape(CollisionShape::Sphere, {}, radius, 0.0f));
    return addBody(std::move(shape), position, mass);
}

void PhysicsSystem::ApplyForce(btRigidBody* body, const glm::vec3& force) {
    if (body) {
        body->activate(true);
        body->applyCentralForce(btVector3(force.x, force.y, force.z));
    }
}
