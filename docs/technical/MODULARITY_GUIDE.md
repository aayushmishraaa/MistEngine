# MistEngine Modularity & Scene Editor Guide

## ?? Overview

MistEngine now features a Unity-like scene editor with modular architecture that allows for post-build extensibility. This guide covers the new input management system and module loading capabilities.

## ?? Scene Editor Controls

### Camera Navigation (Unity-Style)
- **Right Mouse Button + WASD**: Fly camera movement
  - `W` - Move forward
  - `S` - Move backward  
  - `A` - Move left
  - `D` - Move right
- **Right Mouse Button + QE**: Vertical movement
  - `Q` - Move down
  - `E` - Move up
- **Mouse Scroll**: Zoom in/out
- **F3**: Toggle between Scene Editor and Gameplay modes

### Scene Editor vs Gameplay Mode
- **Scene Editor Mode** (Default):
  - Mouse cursor is visible and free
  - Camera only responds when right mouse button is held
  - UI is always accessible
  - Perfect for building and editing scenes

- **Gameplay Mode**:
  - Mouse cursor is locked and hidden
  - Camera responds to all mouse movement
  - Traditional FPS-style controls

### Legacy Physics Controls
- **IJKL + Space**: Control physics cube
  - `I` - Push forward
  - `K` - Push backward
  - `J` - Push left
  - `L` - Push right
  - `Space` - Push up

## ?? Modularity System

### What is it?
The modularity system allows you to extend MistEngine with custom components, systems, and functionality through dynamically loaded libraries (DLLs). This means you can add new features to the engine **after it's built**, just like Unity plugins.

### Key Features
- **Hot Loading**: Load modules at runtime without recompiling the engine
- **Type System Integration**: Custom components integrate seamlessly with ECS
- **Dependency Management**: Modules can depend on other modules
- **Hot Reload**: Modules can be reloaded when files change (development feature)

### Creating a Module

#### 1. Basic Module Structure
```cpp
// MyModule.h
#include "ModuleManager.h"

class MyModule : public IComponentModule {
public:
    MyModule();
    virtual ~MyModule();

    // Required IModule interface
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    ModuleInfo GetInfo() const override;
    bool IsInitialized() const override;

    // IComponentModule interface
    void RegisterComponents(Coordinator* coordinator) override;

private:
    bool m_Initialized;
};

// Export the module
DECLARE_MODULE(MyModule)
```

#### 2. Module Implementation
```cpp
// MyModule.cpp
#include "MyModule.h"

DECLARE_MODULE(MyModule) // This makes it loadable as DLL

bool MyModule::Initialize() {
    std::cout << "MyModule initializing..." << std::endl;
    m_Initialized = true;
    return true;
}

ModuleInfo MyModule::GetInfo() const {
    ModuleInfo info;
    info.name = "MyModule";
    info.version = "1.0.0";
    info.author = "Your Name";
    info.description = "My awesome module";
    info.type = ModuleType::COMPONENT;
    info.interfaceVersion = MODULE_INTERFACE_VERSION;
    info.dependencies = {}; // List other required modules
    return info;
}

void MyModule::RegisterComponents(Coordinator* coordinator) {
    // Register your custom components with ECS
    coordinator->RegisterComponent<MyCustomComponent>();
}
```

#### 3. Custom Components
```cpp
struct MyCustomComponent {
    std::string data;
    float value;
    bool isActive;
    
    MyCustomComponent() : data(""), value(0.0f), isActive(true) {}
};
```

#### 4. Building as DLL
- Set project type to `Dynamic Library (.dll)`
- Add `DECLARE_MODULE(YourClassName)` macro
- Build and place `.dll` in `modules/` directory
- Engine will auto-discover and load it

### Module Types

#### Component Modules
- Add new component types to the ECS
- Implement `IComponentModule` interface
- Use `RegisterComponents()` to register with coordinator

#### System Modules  
- Add new systems that process components
- Implement `ISystemModule` interface
- Use `RegisterSystems()` to add processing logic

#### Renderer Extensions
- Add custom rendering capabilities
- Access to renderer and shader systems
- Custom post-processing effects

#### Script Modules
- Embed scripting languages (Lua, Python, etc.)
- Runtime script execution
- Bindings to engine systems

#### Tool Modules
- Custom editor tools and windows
- Asset importers/exporters
- Debugging utilities

### Loading Modules

#### Automatic Discovery
```cpp
// Engine automatically loads from modules/ directory
moduleManager.LoadModulesFromDirectory("modules");
```

#### Manual Loading
```cpp
// Load specific module
ModuleLoadResult result = moduleManager.LoadModule("MyModule.dll");
if (result.success) {
    std::cout << "Module loaded successfully!" << std::endl;
} else {
    std::cout << "Failed: " << result.errorMessage << std::endl;
}
```

#### Runtime Access
```cpp
// Get loaded module
auto module = moduleManager.GetModule("MyModule");
if (module) {
    // Use module...
}

// Get modules by type
auto componentModules = moduleManager.GetModulesByType(ModuleType::COMPONENT);
```

### Hot Reload (Development)
```cpp
// Enable hot reload for development
moduleManager.EnableHotReload(true);

// Engine automatically detects file changes and reloads modules
```

## ??? Integration Examples

### Adding Custom Components at Runtime
```cpp
// In your module's RegisterComponents function
void MyModule::RegisterComponents(Coordinator* coordinator) {
    coordinator->RegisterComponent<HealthComponent>();
    coordinator->RegisterComponent<InventoryComponent>();
    coordinator->RegisterComponent<DialogueComponent>();
}
```

### Custom Systems
```cpp
class HealthSystem {
public:
    void Update(float deltaTime) {
        // Process all entities with HealthComponent
        for (auto entity : m_Entities) {
            auto& health = coordinator->GetComponent<HealthComponent>(entity);
            if (health.current <= 0) {
                // Handle death
            }
        }
    }
};
```

### Engine Integration
```cpp
// Modules have access to core engine systems
void MyModule::Initialize() {
    // Access ECS coordinator
    auto coordinator = GetCoordinator();
    
    // Access scene
    auto scene = GetScene();
    
    // Access renderer
    auto renderer = GetRenderer();
}
```

## ?? Getting Started

### 1. Try the Example Module
1. Build the `ExampleModule` project
2. Copy `ExampleModule.dll` to `modules/` directory  
3. Run MistEngine - it will auto-load the module
4. Check console for module messages

### 2. Create Your First Module
1. Copy the `ExampleModule` project structure
2. Rename and modify for your needs
3. Implement your custom components/systems
4. Build as DLL and place in `modules/`
5. Engine loads it automatically!

### 3. Scene Editor Workflow
1. Start MistEngine (Scene Editor mode is default)
2. Use Right Mouse + WASD to navigate the scene
3. Use the Hierarchy and Inspector windows to manage objects
4. Press F3 to test in Gameplay mode
5. Return to Scene Editor mode to continue editing

## ?? Directory Structure
```
MistEngine/
??? modules/                    # Auto-loaded modules
?   ??? ExampleModule.dll
?   ??? YourModule.dll
??? src/                       # Engine source
??? include/                   # Engine headers
?   ??? InputManager.h        # New input system
?   ??? ModuleManager.h       # Module system
??? build/                    # Build output
```

## ?? Debugging Modules
- Use console output for logging
- Check module load messages
- Verify dependencies are satisfied
- Ensure interface version matches
- Check file permissions on DLLs

## ?? Best Practices
1. **Keep modules focused** - One responsibility per module
2. **Handle dependencies** - List required modules in ModuleInfo
3. **Cleanup properly** - Implement Shutdown() correctly
4. **Version your interface** - Increment MODULE_INTERFACE_VERSION for breaking changes
5. **Test hot reload** - Ensure modules can be loaded/unloaded cleanly

---

This modular architecture transforms MistEngine from a static engine into a flexible, extensible platform that can grow with your project needs!