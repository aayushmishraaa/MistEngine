#include <catch2/catch_all.hpp>

#include "Core/CommandQueue.h"

#include <atomic>
#include <thread>
#include <vector>

TEST_CASE("CommandQueue runs every pushed command exactly once", "[command_queue]") {
    Mist::CommandQueue q;
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        q.Push([&] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    REQUIRE(q.Size() == 100);

    q.Flush();
    REQUIRE(counter.load() == 100);
    REQUIRE(q.Size() == 0);

    q.Flush();
    REQUIRE(counter.load() == 100); // second flush is a no-op
}

TEST_CASE("CommandQueue is safe under concurrent producers", "[command_queue]") {
    Mist::CommandQueue q;
    std::atomic<int> counter{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 50; ++i) {
                q.Push([&] { counter.fetch_add(1, std::memory_order_relaxed); });
            }
        });
    }
    for (auto& th : threads) th.join();

    REQUIRE(q.Size() == 4 * 50);
    q.Flush();
    REQUIRE(counter.load() == 200);
}

TEST_CASE("CommandQueue Push during Flush defers to next flush", "[command_queue]") {
    Mist::CommandQueue q;
    int runs = 0;
    q.Push([&] {
        ++runs;
        q.Push([&] { ++runs; });  // re-entrant push
    });

    q.Flush();
    // First flush runs the original (+1) and queues the re-entrant one.
    REQUIRE(runs == 1);

    q.Flush();
    REQUIRE(runs == 2);
}
