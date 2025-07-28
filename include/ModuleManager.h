#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#define MODULE_HANDLE HMODULE
#define LOAD_MODULE(path) LoadLibraryA(path)
#define GET_MODULE_FUNCTION(handle, name) GetProcAddress(handle, name)
#define UNLOAD_MODULE(handle) FreeLibrary(handle)
#else
#include <dlfcn.h>
#define MODULE_HANDLE void*
#define LOAD_MODULE(path) dlopen(path, RTLD_LAZY)
#define GET_MODULE_FUNCTION(handle, name) dlsym(handle, name)
#define UNLOAD_MODULE(handle) dlclose(handle)
#endif

// Forward declarations
class Coordinator;
class Scene;
class Renderer;

// Module interface version - increment when breaking changes are made
#define MODULE_INTERFACE_VERSION 1

// Module types
enum class ModuleType {
    COMPONENT,
    SYSTEM,
    RENDERER_EXTENSION,
    SCRIPT,
    TOOL,
    UNKNOWN
};

// Module information structure
struct ModuleInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    ModuleType type;
    int interfaceVersion;
    std::vector<std::string> dependencies;
};

// Base module interface
class IModule {
public:
    virtual ~IModule() = default;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual ModuleInfo GetInfo() const = 0;
    virtual bool IsInitialized() const = 0;
};

// Module loading result
struct ModuleLoadResult {
    bool success;
    std::string errorMessage;
    std::shared_ptr<IModule> module;
};

// C-style function pointers for module loading
typedef IModule* (*CreateModuleFunc)();
typedef void (*DestroyModuleFunc)(IModule*);
typedef int (*GetInterfaceVersionFunc)();

// Loaded module data
struct LoadedModule {
    MODULE_HANDLE handle;
    std::shared_ptr<IModule> module;
    CreateModuleFunc createFunc;
    DestroyModuleFunc destroyFunc;
    std::string filePath;
    ModuleInfo info;
    bool initialized;
};

class ModuleManager {
public:
    ModuleManager();
    ~ModuleManager();

    // Module loading/unloading
    ModuleLoadResult LoadModule(const std::string& filePath);
    bool UnloadModule(const std::string& moduleName);
    void UnloadAllModules();

    // Module discovery
    std::vector<std::string> DiscoverModules(const std::string& directory);
    bool LoadModulesFromDirectory(const std::string& directory);

    // Module management
    std::shared_ptr<IModule> GetModule(const std::string& name) const;
    std::vector<std::shared_ptr<IModule>> GetModulesByType(ModuleType type) const;
    std::vector<ModuleInfo> GetLoadedModuleInfos() const;
    bool IsModuleLoaded(const std::string& name) const;

    // Engine integration
    void SetCoordinator(Coordinator* coordinator) { m_Coordinator = coordinator; }
    void SetScene(Scene* scene) { m_Scene = scene; }
    void SetRenderer(Renderer* renderer) { m_Renderer = renderer; }

    // Module lifecycle
    void UpdateModules(float deltaTime);
    void InitializeModules();
    void ShutdownModules();

    // Hot reloading
    void EnableHotReload(bool enable) { m_HotReloadEnabled = enable; }
    void CheckForModuleChanges();

    // Event system for modules
    template<typename... Args>
    void BroadcastEvent(const std::string& eventName, Args&&... args);

private:
    std::unordered_map<std::string, LoadedModule> m_LoadedModules;
    std::vector<std::string> m_ModuleDirectories;
    
    // Engine references for modules
    Coordinator* m_Coordinator;
    Scene* m_Scene;
    Renderer* m_Renderer;
    
    // Hot reload system
    bool m_HotReloadEnabled;
    std::unordered_map<std::string, long long> m_ModuleTimestamps;
    
    // Helper functions
    bool ValidateModule(MODULE_HANDLE handle, const std::string& filePath);
    std::string GetModuleExtension() const;
    bool CheckDependencies(const ModuleInfo& moduleInfo);
    long long GetFileTimestamp(const std::string& filePath);
};

// Helper macros for module creation
#define DECLARE_MODULE(className) \
    extern "C" { \
        __declspec(dllexport) IModule* CreateModule() { \
            return new className(); \
        } \
        __declspec(dllexport) void DestroyModule(IModule* module) { \
            delete module; \
        } \
        __declspec(dllexport) int GetInterfaceVersion() { \
            return MODULE_INTERFACE_VERSION; \
        } \
    }

// Example module interface for components
class IComponentModule : public IModule {
public:
    virtual void RegisterComponents(Coordinator* coordinator) = 0;
};

// Example module interface for systems
class ISystemModule : public IModule {
public:
    virtual void RegisterSystems(Coordinator* coordinator) = 0;
};

#endif // MODULE_MANAGER_H