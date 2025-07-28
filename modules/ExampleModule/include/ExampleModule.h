#ifndef EXAMPLE_MODULE_H
#define EXAMPLE_MODULE_H

#include "ModuleManager.h"
#include <string>

// Example component module that adds custom functionality
class ExampleModule : public IComponentModule {
public:
    ExampleModule();
    virtual ~ExampleModule();

    // IModule interface
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    ModuleInfo GetInfo() const override;
    bool IsInitialized() const override;

    // IComponentModule interface
    void RegisterComponents(Coordinator* coordinator) override;

private:
    bool m_Initialized;
    float m_UpdateTimer;
};

// Example custom component that could be added via module
struct CustomComponent {
    std::string customData;
    float customValue;
    bool isActive;
    
    CustomComponent() : customData(""), customValue(0.0f), isActive(true) {}
};

// Example system that could be added via module
class CustomSystem {
public:
    void Update(float deltaTime);
    void ProcessCustomComponents();
};

#endif // EXAMPLE_MODULE_H