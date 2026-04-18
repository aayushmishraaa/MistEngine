#include <catch2/catch_all.hpp>

#include "Script/ScriptLanguage.h"
#include "Script/ScriptRegistry.h"

#include <memory>
#include <string>
#include <string_view>

namespace {

// Minimal language impl for tests — tracks Init/Shutdown calls so we can
// confirm the registry invokes lifecycle methods correctly.
struct DummyInstance : Mist::Script::IScriptInstance {
    int& callCounter;
    explicit DummyInstance(int& c) : callCounter(c) {}
    void CallVoid(std::string_view /*method*/) override { ++callCounter; }
};

struct DummyLanguage : Mist::Script::IScriptLanguage {
    int  initCalls     = 0;
    int  shutdownCalls = 0;
    int  callCounter   = 0;
    std::string_view GetName()      const override { return "Dummy"; }
    std::string_view GetExtension() const override { return ".dummy"; }
    std::unique_ptr<Mist::Script::IScriptInstance> Compile(std::string_view /*src*/) override {
        return std::make_unique<DummyInstance>(callCounter);
    }
    void Init()     override { ++initCalls; }
    void Shutdown() override { ++shutdownCalls; }
};

} // namespace

TEST_CASE("ScriptRegistry resolves by extension + calls Init", "[script]") {
    auto lang = std::make_shared<DummyLanguage>();
    Mist::Script::ScriptRegistry::Instance().Register(lang);

    REQUIRE(lang->initCalls == 1);

    auto found = Mist::Script::ScriptRegistry::Instance().Get(".dummy");
    REQUIRE(found != nullptr);
    REQUIRE(found->GetName() == "Dummy");

    // Case-insensitive lookup matches Godot's convention.
    auto foundUpper = Mist::Script::ScriptRegistry::Instance().Get(".DUMMY");
    REQUIRE(foundUpper != nullptr);

    // Missing extension → null, not a throw.
    auto notFound = Mist::Script::ScriptRegistry::Instance().Get(".zzz-never-registered");
    REQUIRE(notFound == nullptr);
}

TEST_CASE("ScriptInstance::CallVoid routes to backend", "[script]") {
    auto lang = std::make_shared<DummyLanguage>();
    Mist::Script::ScriptRegistry::Instance().Register(lang);

    auto inst = lang->Compile("unused");
    REQUIRE(inst != nullptr);
    inst->CallVoid("anything");
    inst->CallVoid("anything");
    REQUIRE(lang->callCounter == 2);
}
