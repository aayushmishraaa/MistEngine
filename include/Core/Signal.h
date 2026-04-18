#pragma once
#ifndef MIST_SIGNAL_H
#define MIST_SIGNAL_H

// First-class event bus modelled on Godot's signal system but kept minimal:
// declare a `Signal<Args...>` member or static; `Connect(callback)` returns
// a handle that can be passed to `Disconnect` later. `Emit(args...)` fires
// every connected callback in registration order.
//
// Thread-safety: guarded by an internal mutex. A connection made during
// Emit on the same thread is queued and applied at the start of the next
// Emit; this mirrors Godot's behaviour and avoids iterator invalidation
// during dispatch.

#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace Mist {

template <typename... Args>
class Signal {
public:
    using Callback   = std::function<void(Args...)>;
    using Connection = std::size_t;

    // Register a callback. Returns an opaque connection ID that remains
    // stable across later Connect/Disconnect calls — caller stashes it
    // somewhere if they ever want to Disconnect.
    Connection Connect(Callback cb) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        const Connection id = m_NextId++;
        if (m_Emitting) {
            // Queue the insert; flushed at the next Emit.
            m_PendingAdds.push_back({id, std::move(cb)});
        } else {
            m_Slots.push_back({id, std::move(cb)});
        }
        return id;
    }

    // Remove a connection. Safe to call from inside a callback — the
    // removal takes effect after the current Emit completes.
    void Disconnect(Connection id) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_Emitting) {
            m_PendingRemoves.push_back(id);
            return;
        }
        eraseId(id);
    }

    // Fire every connected callback in registration order. Exceptions in
    // callbacks propagate through (caller's problem) — we just make sure
    // the emitting flag and pending-ops queues stay consistent.
    void Emit(Args... args) {
        std::unique_lock<std::mutex> lock(m_Mutex);
        // Snapshot slots under lock so a concurrent Connect/Disconnect
        // doesn't invalidate the iteration.
        auto snapshot = m_Slots;
        m_Emitting = true;
        lock.unlock();

        for (auto& [id, cb] : snapshot) {
            cb(args...);
        }

        lock.lock();
        m_Emitting = false;
        // Apply deferred ops.
        for (auto id : m_PendingRemoves) eraseId(id);
        m_PendingRemoves.clear();
        for (auto& p : m_PendingAdds) m_Slots.push_back(std::move(p));
        m_PendingAdds.clear();
    }

    std::size_t ConnectionCount() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Slots.size();
    }

private:
    struct Slot { Connection id; Callback cb; };

    void eraseId(Connection id) {
        for (auto it = m_Slots.begin(); it != m_Slots.end(); ++it) {
            if (it->id == id) { m_Slots.erase(it); return; }
        }
    }

    mutable std::mutex m_Mutex;
    std::vector<Slot>  m_Slots;
    std::vector<Slot>  m_PendingAdds;
    std::vector<Connection> m_PendingRemoves;
    Connection m_NextId  = 1;
    bool       m_Emitting = false;
};

} // namespace Mist

#endif // MIST_SIGNAL_H
