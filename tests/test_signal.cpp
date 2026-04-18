#include <catch2/catch_all.hpp>

#include "Core/Signal.h"

#include <string>
#include <vector>

TEST_CASE("Signal fires registered callbacks in order", "[signal]") {
    Mist::Signal<int> sig;
    std::vector<int> seen;

    sig.Connect([&](int x) { seen.push_back(x + 1); });
    sig.Connect([&](int x) { seen.push_back(x * 10); });

    sig.Emit(5);

    REQUIRE(seen == std::vector<int>{6, 50});
}

TEST_CASE("Signal Disconnect stops further calls", "[signal]") {
    Mist::Signal<> sig;
    int calls = 0;
    auto id = sig.Connect([&] { ++calls; });

    sig.Emit();
    REQUIRE(calls == 1);

    sig.Disconnect(id);
    sig.Emit();
    REQUIRE(calls == 1);
}

TEST_CASE("Signal supports re-entrant Connect/Disconnect", "[signal]") {
    // Callbacks that mutate the signal from within Emit should not
    // corrupt iteration. The queued-change design applies the mutation
    // *after* the current Emit completes.
    Mist::Signal<> sig;
    int base = 0;
    Mist::Signal<>::Connection baseId = 0;
    baseId = sig.Connect([&] {
        ++base;
        sig.Disconnect(baseId);                     // remove myself next time
        sig.Connect([&] { base += 100; });          // add a new listener
    });

    sig.Emit();
    REQUIRE(base == 1);

    sig.Emit();
    // Original callback is gone; new listener (+100) fires.
    REQUIRE(base == 101);
}

TEST_CASE("Signal with typed args carries values correctly", "[signal]") {
    Mist::Signal<std::string, int> sig;
    std::string lastMsg;
    int lastN = 0;
    sig.Connect([&](std::string s, int n) {
        lastMsg = std::move(s);
        lastN   = n;
    });

    sig.Emit("hello", 42);
    REQUIRE(lastMsg == "hello");
    REQUIRE(lastN == 42);
}
