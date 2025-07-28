#include "ModuleManager.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <dlfcn.h>
#include <dirent.h>
#endif

ModuleManager::ModuleManager()
    : m_Coordinator(nullptr)
    , m_Scene(nullptr)
    , m_Renderer(nullptr)
    , m_HotReloadEnabled(false)
{
}

ModuleManager::~ModuleManager() {
    UnloadAllModules();
}

ModuleLoadResult ModuleManager::LoadModule(const std::string& filePath) {
    ModuleLoadResult result;
    result.success = false;

    // Extract module name from file path (simple approach for C++14)
    size_t lastSlash = filePath.find_last_of("/\\");
    size_t lastDot = filePath.find_last_of('.');
    std::string moduleName = filePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    
    if (IsModuleLoaded(moduleName)) {
        result.errorMessage = "Module '" + moduleName + "' is already loaded";
        return result;
    }

    // Load the dynamic library
    MODULE_HANDLE handle = LOAD_MODULE(filePath.c_str());
    if (!handle) {
#ifdef _WIN32
        DWORD error = GetLastError();
        result.errorMessage = "Failed to load module: " + filePath + " (Error: " + std::to_string(error) + ")";
#else
        result.errorMessage = "Failed to load module: " + filePath + " (" + std::string(dlerror()) + ")";
#endif
        return result;
    }

    // Validate module interface
    if (!ValidateModule(handle, filePath)) {
        result.errorMessage = "Invalid module interface: " + filePath;
        UNLOAD_MODULE(handle);
        return result;
    }

    // Get module functions
    CreateModuleFunc createFunc = reinterpret_cast<CreateModuleFunc>(
        GET_MODULE_FUNCTION(handle, "CreateModule"));
    DestroyModuleFunc destroyFunc = reinterpret_cast<DestroyModuleFunc>(
        GET_MODULE_FUNCTION(handle, "DestroyModule"));

    if (!createFunc || !destroyFunc) {
        result.errorMessage = "Module missing required functions: " + filePath;
        UNLOAD_MODULE(handle);
        return result;
    }

    // Create module instance
    IModule* modulePtr = createFunc();
    if (!modulePtr) {
        result.errorMessage = "Failed to create module instance: " + filePath;
        UNLOAD_MODULE(handle);
        return result;
    }

    std::shared_ptr<IModule> module(modulePtr, [destroyFunc](IModule* m) {
        destroyFunc(m);
    });

    // Get module info
    ModuleInfo info = module->GetInfo();
    
    // Check dependencies
    if (!CheckDependencies(info)) {
        result.errorMessage = "Module dependencies not satisfied: " + info.name;
        UNLOAD_MODULE(handle);
        return result;
    }

    // Store loaded module
    LoadedModule loadedModule;
    loadedModule.handle = handle;
    loadedModule.module = module;
    loadedModule.createFunc = createFunc;
    loadedModule.destroyFunc = destroyFunc;
    loadedModule.filePath = filePath;
    loadedModule.info = info;
    loadedModule.initialized = false;

    m_LoadedModules[info.name] = loadedModule;

    // Store timestamp for hot reload
    if (m_HotReloadEnabled) {
        m_ModuleTimestamps[filePath] = GetFileTimestamp(filePath);
    }

    result.success = true;
    result.module = module;
    
    std::cout << "Successfully loaded module: " << info.name << " v" << info.version << std::endl;
    return result;
}

bool ModuleManager::UnloadModule(const std::string& moduleName) {
    auto it = m_LoadedModules.find(moduleName);
    if (it == m_LoadedModules.end()) {
        return false;
    }

    LoadedModule& loadedModule = it->second;
    
    // Shutdown module if initialized
    if (loadedModule.initialized && loadedModule.module) {
        loadedModule.module->Shutdown();
    }

    // Unload the library
    if (loadedModule.handle) {
        UNLOAD_MODULE(loadedModule.handle);
    }

    // Remove from hot reload tracking
    if (m_HotReloadEnabled) {
        m_ModuleTimestamps.erase(loadedModule.filePath);
    }

    m_LoadedModules.erase(it);
    std::cout << "Unloaded module: " << moduleName << std::endl;
    return true;
}

void ModuleManager::UnloadAllModules() {
    // Shutdown all modules first
    ShutdownModules();
    
    // Then unload them
    for (auto& pair : m_LoadedModules) {
        LoadedModule& loadedModule = pair.second;
        if (loadedModule.handle) {
            UNLOAD_MODULE(loadedModule.handle);
        }
    }
    
    m_LoadedModules.clear();
    m_ModuleTimestamps.clear();
    std::cout << "Unloaded all modules" << std::endl;
}

std::vector<std::string> ModuleManager::DiscoverModules(const std::string& directory) {
    std::vector<std::string> moduleFiles;
    std::string extension = GetModuleExtension();
    
#ifdef _WIN32
    // Windows-specific directory iteration
    WIN32_FIND_DATAA findData;
    std::string searchPath = directory + "\\*" + extension;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string fileName = findData.cFileName;
            if (fileName.length() > extension.length() && 
                fileName.substr(fileName.length() - extension.length()) == extension) {
                moduleFiles.push_back(directory + "\\" + fileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    // Unix-like systems
    DIR* dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string fileName = entry->d_name;
            if (fileName.length() > extension.length() && 
                fileName.substr(fileName.length() - extension.length()) == extension) {
                moduleFiles.push_back(directory + "/" + fileName);
            }
        }
        closedir(dir);
    }
#endif
    
    return moduleFiles;
}

bool ModuleManager::LoadModulesFromDirectory(const std::string& directory) {
    std::vector<std::string> moduleFiles = DiscoverModules(directory);
    bool allLoaded = true;
    
    for (const std::string& filePath : moduleFiles) {
        ModuleLoadResult result = LoadModule(filePath);
        if (!result.success) {
            std::cerr << "Failed to load module " << filePath << ": " << result.errorMessage << std::endl;
            allLoaded = false;
        }
    }
    
    if (!moduleFiles.empty()) {
        InitializeModules();
    }
    
    return allLoaded;
}

std::shared_ptr<IModule> ModuleManager::GetModule(const std::string& name) const {
    auto it = m_LoadedModules.find(name);
    if (it != m_LoadedModules.end()) {
        return it->second.module;
    }
    return nullptr;
}

std::vector<std::shared_ptr<IModule>> ModuleManager::GetModulesByType(ModuleType type) const {
    std::vector<std::shared_ptr<IModule>> modules;
    
    for (const auto& pair : m_LoadedModules) {
        if (pair.second.info.type == type) {
            modules.push_back(pair.second.module);
        }
    }
    
    return modules;
}

std::vector<ModuleInfo> ModuleManager::GetLoadedModuleInfos() const {
    std::vector<ModuleInfo> infos;
    
    for (const auto& pair : m_LoadedModules) {
        infos.push_back(pair.second.info);
    }
    
    return infos;
}

bool ModuleManager::IsModuleLoaded(const std::string& name) const {
    return m_LoadedModules.find(name) != m_LoadedModules.end();
}

void ModuleManager::UpdateModules(float deltaTime) {
    for (auto& pair : m_LoadedModules) {
        LoadedModule& loadedModule = pair.second;
        if (loadedModule.initialized && loadedModule.module) {
            loadedModule.module->Update(deltaTime);
        }
    }
    
    // Check for hot reload if enabled
    if (m_HotReloadEnabled) {
        CheckForModuleChanges();
    }
}

void ModuleManager::InitializeModules() {
    // Sort modules by dependencies (simple approach - can be improved)
    std::vector<std::string> initOrder;
    std::vector<std::string> remaining;
    
    for (const auto& pair : m_LoadedModules) {
        remaining.push_back(pair.first);
    }
    
    // Simple dependency resolution (topological sort would be better)
    while (!remaining.empty()) {
        bool progress = false;
        
        for (auto it = remaining.begin(); it != remaining.end(); ) {
            const std::string& moduleName = *it;
            const LoadedModule& loadedModule = m_LoadedModules[moduleName];
            
            bool dependenciesSatisfied = true;
            for (const std::string& dep : loadedModule.info.dependencies) {
                if (std::find(initOrder.begin(), initOrder.end(), dep) == initOrder.end()) {
                    dependenciesSatisfied = false;
                    break;
                }
            }
            
            if (dependenciesSatisfied) {
                initOrder.push_back(moduleName);
                it = remaining.erase(it);
                progress = true;
            } else {
                ++it;
            }
        }
        
        if (!progress && !remaining.empty()) {
            std::cerr << "Circular dependency detected in modules" << std::endl;
            break;
        }
    }
    
    // Initialize modules in order
    for (const std::string& moduleName : initOrder) {
        LoadedModule& loadedModule = m_LoadedModules[moduleName];
        if (!loadedModule.initialized && loadedModule.module) {
            if (loadedModule.module->Initialize()) {
                loadedModule.initialized = true;
                std::cout << "Initialized module: " << moduleName << std::endl;
            } else {
                std::cerr << "Failed to initialize module: " << moduleName << std::endl;
            }
        }
    }
}

void ModuleManager::ShutdownModules() {
    // Shutdown in reverse order
    std::vector<std::string> shutdownOrder;
    for (const auto& pair : m_LoadedModules) {
        if (pair.second.initialized) {
            shutdownOrder.push_back(pair.first);
        }
    }
    
    std::reverse(shutdownOrder.begin(), shutdownOrder.end());
    
    for (const std::string& moduleName : shutdownOrder) {
        LoadedModule& loadedModule = m_LoadedModules[moduleName];
        if (loadedModule.module) {
            loadedModule.module->Shutdown();
            loadedModule.initialized = false;
            std::cout << "Shutdown module: " << moduleName << std::endl;
        }
    }
}

void ModuleManager::CheckForModuleChanges() {
    for (auto& pair : m_ModuleTimestamps) {
        const std::string& filePath = pair.first;
        long long& storedTimestamp = pair.second;
        
        long long currentTimestamp = GetFileTimestamp(filePath);
        if (currentTimestamp != storedTimestamp && currentTimestamp != -1) {
            std::cout << "Module changed detected: " << filePath << std::endl;
            // TODO: Implement hot reload logic
            storedTimestamp = currentTimestamp;
        }
    }
}

bool ModuleManager::ValidateModule(MODULE_HANDLE handle, const std::string& filePath) {
    // Check for required functions
    GetInterfaceVersionFunc versionFunc = reinterpret_cast<GetInterfaceVersionFunc>(
        GET_MODULE_FUNCTION(handle, "GetInterfaceVersion"));
    
    if (!versionFunc) {
        return false;
    }
    
    int moduleVersion = versionFunc();
    if (moduleVersion != MODULE_INTERFACE_VERSION) {
        std::cerr << "Module interface version mismatch: " << filePath 
                  << " (expected " << MODULE_INTERFACE_VERSION 
                  << ", got " << moduleVersion << ")" << std::endl;
        return false;
    }
    
    return true;
}

std::string ModuleManager::GetModuleExtension() const {
#ifdef _WIN32
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

bool ModuleManager::CheckDependencies(const ModuleInfo& moduleInfo) {
    for (const std::string& dependency : moduleInfo.dependencies) {
        if (!IsModuleLoaded(dependency)) {
            std::cerr << "Module " << moduleInfo.name << " requires " << dependency << std::endl;
            return false;
        }
    }
    return true;
}

long long ModuleManager::GetFileTimestamp(const std::string& filePath) {
#ifdef _WIN32
    struct _stat result;
    if (_stat(filePath.c_str(), &result) == 0) {
        return result.st_mtime;
    }
#else
    struct stat result;
    if (stat(filePath.c_str(), &result) == 0) {
        return result.st_mtime;
    }
#endif
    return -1;
}