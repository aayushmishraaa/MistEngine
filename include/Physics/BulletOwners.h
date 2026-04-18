#pragma once

#include <btBulletDynamicsCommon.h>
#include <memory>
#include <utility>

namespace Mist::Physics {

// btRigidBody, btMotionState, and btCollisionShape are allocated separately
// in Bullet and have ordering constraints on destruction. These three wrappers
// make that explicit: each one owns exactly one allocation and deletes it
// deterministically when the wrapper dies.
//
// Crucially, ScopedRigidBody's destructor removes the body from its world
// *before* freeing memory — otherwise Bullet's solver keeps a dangling
// pointer and any subsequent stepSimulation() reads freed memory.

struct CollisionShapeDeleter {
    void operator()(btCollisionShape* s) const noexcept { delete s; }
};
using ScopedCollisionShape = std::unique_ptr<btCollisionShape, CollisionShapeDeleter>;

struct MotionStateDeleter {
    void operator()(btMotionState* m) const noexcept { delete m; }
};
using ScopedMotionState = std::unique_ptr<btMotionState, MotionStateDeleter>;

// Non-moveable composite: body + motion state are bound 1:1 because the
// body's constructor captured the motion-state pointer. We remove the body
// from `world` on destruction, which reads from world — so the wrapper is
// pinned to the world it was attached to.
class ScopedRigidBody {
  public:
    ScopedRigidBody(btDiscreteDynamicsWorld* world, std::unique_ptr<btRigidBody> body,
                    ScopedMotionState motionState) noexcept
        : m_World(world), m_Body(std::move(body)), m_MotionState(std::move(motionState)) {}

    ScopedRigidBody(const ScopedRigidBody&) = delete;
    ScopedRigidBody& operator=(const ScopedRigidBody&) = delete;

    ScopedRigidBody(ScopedRigidBody&& other) noexcept
        : m_World(other.m_World), m_Body(std::move(other.m_Body)),
          m_MotionState(std::move(other.m_MotionState)) {
        other.m_World = nullptr;
    }

    ScopedRigidBody& operator=(ScopedRigidBody&& other) noexcept {
        if (this != &other) {
            reset();
            m_World = other.m_World;
            m_Body = std::move(other.m_Body);
            m_MotionState = std::move(other.m_MotionState);
            other.m_World = nullptr;
        }
        return *this;
    }

    ~ScopedRigidBody() { reset(); }

    btRigidBody* get() const noexcept { return m_Body.get(); }

  private:
    void reset() noexcept {
        if (m_World && m_Body) {
            m_World->removeRigidBody(m_Body.get());
        }
        m_Body.reset();        // frees the btRigidBody
        m_MotionState.reset(); // frees its motion state
        m_World = nullptr;
    }

    btDiscreteDynamicsWorld* m_World = nullptr;
    std::unique_ptr<btRigidBody> m_Body;
    ScopedMotionState m_MotionState;
};

} // namespace Mist::Physics
