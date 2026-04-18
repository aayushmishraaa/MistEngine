#pragma once
#ifndef MIST_COMMAND_QUEUE_H
#define MIST_COMMAND_QUEUE_H

#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace Mist {

// Cross-thread command queue. Patterned after Godot's CommandQueueMT:
// the main thread pushes work; the render thread drains it at Flush()
// time. Today the engine is single-threaded and `Flush` runs inline on
// whoever called it, but the API shape is correct for the G9 follow-up
// cycle that moves `Flush` onto a dedicated render thread.
//
// Implementation note: current version uses a mutex + vector<function>.
// Lock-free SPSC ringbuffer is a later optimisation; the interface above
// stays the same.
class CommandQueue {
public:
    // Push a command from any thread. Cheap under contention — we only
    // take the mutex to append to the vector.
    template <typename Fn>
    void Push(Fn&& cmd) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Queue.emplace_back(std::forward<Fn>(cmd));
    }

    // Execute every queued command. Call on the thread that owns the
    // "consumer" context (GL context, in MistEngine's case the main
    // thread today). Swaps the internal buffer so commands Push'd during
    // Flush are deferred to the next Flush and don't recurse.
    void Flush() {
        std::vector<std::function<void()>> drained;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            drained.swap(m_Queue);
        }
        for (auto& cmd : drained) {
            cmd();
        }
    }

    std::size_t Size() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Queue.size();
    }

private:
    mutable std::mutex                 m_Mutex;
    std::vector<std::function<void()>> m_Queue;
};

} // namespace Mist

#endif // MIST_COMMAND_QUEUE_H
