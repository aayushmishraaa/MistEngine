# ?? Creating Your Own MistEngine Module

## ?? **Prerequisites**
- Visual Studio 2017+ with C++ support
- MistEngine source code or development headers
- Basic understanding of C++ and DLLs

## ?? **Quick Start Template**

### **1. Create New DLL Project**
```
File ? New ? Project ? Visual C++ ? Dynamic-Link Library (DLL)
Project Name: MyCustomModule
```

### **2. Module Header (MyCustomModule.h)**
```cpp
#ifndef MY_CUSTOM_MODULE_H
#define MY_CUSTOM_MODULE_H

#include "ModuleManager.h"
#include <string>

class MyCustomModule : public IComponentModule {
public:
    MyCustomModule();
    virtual ~MyCustomModule();

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

// Example custom component
struct MyCustomComponent {
    std::string customName;
    float customValue;
    bool isEnabled;
    
    MyCustomComponent() : customName("Default"), customValue(1.0f), isEnabled(true) {}
    MyCustomComponent(const std::string& name, float value) 
        : customName(name), customValue(value), isEnabled(true) {}
};

#endif // MY_CUSTOM_MODULE_H
```

### **3. Module Implementation (MyCustomModule.cpp)**
```cpp
#include "MyCustomModule.h"
#include <iostream>

MyCustomModule::MyCustomModule() : m_Initialized(false), m_UpdateTimer(0.0f) {
}

MyCustomModule::~MyCustomModule() {
}

bool MyCustomModule::Initialize() {
    if (m_Initialized) return true;
    
    std::cout << "MyCustomModule: Initializing..." << std::endl;
    
    // Your initialization code here
    // - Set up resources
    // - Initialize systems
    // - Prepare data
    
    m_Initialized = true;
    std::cout << "MyCustomModule: Initialized successfully!" << std::endl;
    return true;
}

void MyCustomModule::Shutdown() {
    if (!m_Initialized) return;
    
    std::cout << "MyCustomModule: Shutting down..." << std::endl;
    
    // Your cleanup code here
    // - Release resources
    // - Save data
    // - Cleanup systems
    
    m_Initialized = false;
    std::cout << "MyCustomModule: Shutdown complete!" << std::endl;
}

void MyCustomModule::Update(float deltaTime) {
    if (!m_Initialized) return;
    
    // Your per-frame update code here
    m_UpdateTimer += deltaTime;
    
    // Example: Do something every 5 seconds
    if (m_UpdateTimer >= 5.0f) {
        std::cout << "MyCustomModule: 5 second update tick!" << std::endl;
        m_UpdateTimer = 0.0f;
    }
}

ModuleInfo MyCustomModule::GetInfo() const {
    ModuleInfo info;
    info.name = "MyCustomModule";
    info.version = "1.0.0";
    info.author = "Your Name";
    info.description = "A custom module that demonstrates the MistEngine module system";
    info.type = ModuleType::COMPONENT;
    info.interfaceVersion = MODULE_INTERFACE_VERSION;
    // info.dependencies = {"SomeOtherModule"}; // If you depend on other modules
    return info;
}

bool MyCustomModule::IsInitialized() const {
    return m_Initialized;
}

void MyCustomModule::RegisterComponents(Coordinator* coordinator) {
    if (!coordinator) {
        std::cerr << "MyCustomModule: No coordinator provided!" << std::endl;
        return;
    }
    
    std::cout << "MyCustomModule: Registering components..." << std::endl;
    
    // Register your custom component with the ECS
    coordinator->RegisterComponent<MyCustomComponent>();
    
    std::cout << "MyCustomModule: Components registered!" << std::endl;
}

// REQUIRED: Export functions for module loading
DECLARE_MODULE(MyCustomModule)
```

### **4. Project Configuration**

#### **Include Directories**
Add to project settings:
```
Configuration Properties ? C/C++ ? General ? Additional Include Directories:
- $(SolutionDir)include
- $(SolutionDir)include\ECS
```

#### **Library Dependencies**
```
Configuration Properties ? Linker ? Input ? Additional Dependencies:
- (No additional libraries needed for basic modules)
```

#### **Output Settings**
```
Configuration Properties ? General ? Output Directory: $(SolutionDir)modules\
Configuration Properties ? General ? Target Name: $(ProjectName)
```

---

## ??? **Building Your Module**

### **1. Build Module**
```bash
# Build in Visual Studio or command line:
msbuild MyCustomModule.vcxproj /p:Configuration=Release /p:Platform=x64
```

### **2. Test Module**
```bash
# Module DLL should be created in modules/ directory
# Start MistEngine to test loading
```

### **3. Debug Module**
```cpp
// Add debug output to see module loading:
std::cout << "MyCustomModule: Function called!" << std::endl;
```

---

## ?? **Module Types & Examples**

### **Component Module (Adds new ECS components)**
```cpp
class MyComponentModule : public IComponentModule {
    void RegisterComponents(Coordinator* coordinator) override {
        coordinator->RegisterComponent<HealthComponent>();
        coordinator->RegisterComponent<InventoryComponent>();
    }
};
```

### **System Module (Adds new ECS systems)**
```cpp
class MySystemModule : public ISystemModule {
    void RegisterSystems(Coordinator* coordinator) override {
        auto healthSystem = coordinator->RegisterSystem<HealthSystem>();
        auto inventorySystem = coordinator->RegisterSystem<InventorySystem>();
        
        // Set up system signatures
        Signature healthSig;
        healthSig.set(coordinator->GetComponentType<HealthComponent>());
        coordinator->SetSystemSignature<HealthSystem>(healthSig);
    }
};
```

### **Renderer Extension Module**
```cpp
class MyRendererModule : public IModule {
    void Initialize() override {
        // Add custom shaders, materials, effects
    }
    
    void Update(float deltaTime) override {
        // Custom rendering logic
    }
};
```

---

## ?? **Distribution**

### **1. Package Your Module**
```
MyCustomModule-v1.0.zip
??? MyCustomModule.dll
??? README.md
??? CHANGELOG.md
??? examples/
    ??? usage_example.cpp
```

### **2. Create Module README**
```markdown
# MyCustomModule

## What it does
This module adds custom functionality to MistEngine...

## Installation
1. Download MyCustomModule.dll
2. Copy to MistEngine/modules/ folder
3. Restart MistEngine

## Usage
- Creates new components: MyCustomComponent
- Adds new functionality: ...

## Configuration
No configuration required.
```

### **3. Share Your Module**
- Upload to GitHub
- Share in MistEngine community
- Submit to future module marketplace

---

## ?? **Troubleshooting**

### **Module Not Loading**
1. Check console output for error messages
2. Verify DLL is in modules/ directory
3. Ensure x64 platform build
4. Check module interface version

### **Common Issues**
```cpp
// Missing exports - add this:
DECLARE_MODULE(YourModuleClass)

// Wrong interface version - update:
info.interfaceVersion = MODULE_INTERFACE_VERSION;

// Platform mismatch - ensure x64 build:
Configuration Manager ? Active Solution Platform ? x64
```

### **Debug Tips**
```cpp
// Add logging to track module lifecycle:
std::cout << "Module: Initialize called" << std::endl;
std::cout << "Module: Registering components..." << std::endl;
std::cout << "Module: Update called, deltaTime = " << deltaTime << std::endl;
```

---

## ?? **Advanced Features**

### **Module Communication**
```cpp
// Modules can communicate through the ECS or events
void MyModule::Update(float deltaTime) {
    // Access other modules through coordinator
    auto entities = coordinator->GetEntitiesWithComponent<MyComponent>();
}
```

### **Hot Reloading**
```cpp
// Modules support hot reloading during development
moduleManager.EnableHotReload(true);
// Modules will reload when DLL file changes
```

### **Dependencies**
```cpp
ModuleInfo GetInfo() const override {
    ModuleInfo info;
    // ...
    info.dependencies = {"CorePhysics", "AudioSystem"};
    return info;
}
```

---

## ?? **Next Steps**

1. **Try the template** - Build the example module
2. **Customize** - Add your own components/systems  
3. **Test** - Load in MistEngine and verify functionality
4. **Share** - Distribute your module to the community

**Happy modding!** ???