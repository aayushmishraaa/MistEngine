# ?? MistEngine 0.3.0

[![Version](https://img.shields.io/badge/version-0.3.0--prealpha-brightgreen.svg)](https://github.com/yourusername/MistEngine)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-blue.svg)](https://github.com/yourusername/MistEngine)
[![C++](https://img.shields.io/badge/C%2B%2B-14-blue.svg)](https://github.com/yourusername/MistEngine)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

> **A Modern C++ Game Engine with AI Integration and FPS Game Mode**

MistEngine is a sophisticated 3D game engine built with modern C++14, featuring an Entity-Component-System (ECS) architecture, advanced physics simulation, AI-powered development assistance, and a complete FPS game implementation.

---

## ? **Key Features**

### ?? **Complete FPS Game**
- **Fully playable FPS experience** with enemy AI, weapons, and levels
- **Real-time combat** with physics-based projectiles
- **Intelligent enemy behaviors** including patrol, chase, and attack states
- **Multiple weapon types** (Pistol, Rifle, Shotgun, Sniper)
- **Health and scoring systems** with game progression

### ??? **Advanced Engine Architecture**
- **Entity-Component-System (ECS)** for maximum performance and flexibility
- **Modular plugin system** with hot-reloadable modules
- **Dual physics integration** (Bullet Physics + custom ECS physics)
- **Modern OpenGL 3.3 rendering** with shadow mapping and skybox
- **Scene editor** with transform manipulation and object creation

### ?? **AI-Powered Development**
- **Real-time AI assistance** integrated directly into the engine
- **Google Gemini API support** for intelligent code suggestions
- **Context-aware development help** with F2 hotkey access
- **AI-assisted debugging** and problem-solving

### ?? **Professional Editor Interface**
- **ImGui-based editor** with hierarchy, inspector, and console
- **Game export system** for distributing standalone games
- **Asset browser** and scene management
- **Multiple input modes** (Scene Editor, FPS Game, UI-focused)

### ?? **Developer Experience**
- **Hot-reloadable plugins** for rapid development
- **Comprehensive logging** and debug console
- **Modern C++14 practices** with RAII and smart pointers
- **Cross-platform foundation** with Windows x64 primary support

---

## ?? **Quick Start**

### **Prerequisites**
- Visual Studio 2019 or later with C++14 support
- Windows 10/11 (x64)
- OpenGL 3.3 compatible graphics card
- 4GB RAM minimum, 8GB recommended

### **Build & Run**
```bash
# Clone the repository
git clone https://github.com/yourusername/MistEngine.git
cd MistEngine

# Open the solution file
start MistEngine.sln

# Build and run in Visual Studio (Ctrl+F5)
```

### **First Launch**
1. **?? Try the FPS Game**: Click the green "?? START FPS GAME" button
2. **??? Explore the Editor**: Use GameObject menu to create cubes, spheres, planes
3. **?? Setup AI Assistant**: Press F2 and configure your Gemini API key
4. **?? Read Documentation**: Check the `docs/` folder for comprehensive guides

---

## ?? **FPS Game Controls**

### **Scene Editor Mode**
- `WASD` + `QE` - Camera movement
- `Right-click + Hold` - Mouse look
- `F3` - Toggle between Editor and FPS modes

### **FPS Game Mode** 
- `WASD` - Player movement
- `Mouse` - Look around (automatically captured)
- `Left Click` - Shoot
- `R` - Reload weapon
- `1/2` - Switch weapons
- `ESC` - Pause/Resume game

### **General**
- `F1` - Toggle ImGui demo window
- `F2` - Open AI Assistant
- `F3` - Toggle input modes

---

## ?? **Architecture Overview**

MistEngine implements several advanced software engineering patterns:

### **Entity-Component-System (ECS)**
```cpp
// Create entity with components
Entity player = gCoordinator.CreateEntity();
gCoordinator.AddComponent(player, TransformComponent{});
gCoordinator.AddComponent(player, PlayerComponent{});
gCoordinator.AddComponent(player, WeaponComponent{});
```

### **Modular Plugin System**
```cpp
// Hot-reloadable plugin development
class MyPlugin : public Module {
    void Initialize() override { /* Plugin initialization */ }
    void Update(float deltaTime) override { /* Plugin logic */ }
};
```

### **AI Integration**
```cpp
// Real-time development assistance
AIResponse response = aiManager.GetCodeSuggestions(currentContext);
```

---

## ?? **Project Highlights**

### **?? Advanced Features**
- **Complete FPS Game Implementation** - Playable shooter with AI enemies
- **AI-Powered Development Environment** - Revolutionary coding assistance
- **Professional Game Export System** - Distribute standalone games
- **Hot-Reloadable Plugin Architecture** - Rapid development workflow
- **Dual Physics Systems** - Both Bullet Physics and custom ECS physics
- **Comprehensive Documentation** - Extensive user and technical guides

### **?? Technical Achievements**
- **Modern C++14 Implementation** with advanced template metaprogramming
- **Memory-Safe Architecture** using RAII and smart pointers
- **High-Performance ECS** with optimized component storage
- **Cross-System Integration** between physics, rendering, and AI
- **Professional Build System** with comprehensive error handling

### **?? Academic Quality**
- **MSc Dissertation Level Documentation** in `docs/technical/`
- **Comprehensive Architecture Analysis** with design pattern coverage
- **Research-Quality Implementation** suitable for academic study
- **Extensive Problem-Solving Documentation** showing development process

---

## ?? **Project Structure**

```
MistEngine/
??? ??? src/                    # Source code
?   ??? ECS/                   # Entity-Component-System
?   ??? AI/                    # AI integration system
?   ??? Physics/               # Physics systems
?   ??? ...                    # Core engine systems
??? ?? include/                # Header files
??? ?? shaders/                # GLSL shaders
??? ?? modules/                # Plugin modules
??? ?? docs/                   # Comprehensive documentation
??? ?? exports/                # Game export outputs
??? ?? MistEngine.sln          # Visual Studio solution
```

---

## ?? **AI Assistant Setup**

MistEngine includes revolutionary AI-powered development assistance:

1. **Get API Key**: Visit [Google AI Studio](https://aistudio.google.com/app/apikey)
2. **Configure in Engine**: Press `F2` ? Configure API Key ? Paste key
3. **Start Coding**: Get real-time suggestions, debugging help, and code reviews

**AI Features:**
- ?? Real-time code suggestions
- ?? Intelligent debugging assistance  
- ?? Context-aware documentation
- ?? Code refactoring recommendations

---

## ?? **System Requirements**

### **Minimum**
- Windows 10 x64
- Visual Studio 2019
- OpenGL 3.3 GPU
- 4GB RAM
- 2GB disk space

### **Recommended**
- Windows 11 x64
- Visual Studio 2022
- Modern GPU (GTX 1060+)
- 8GB+ RAM
- SSD storage

---

## ?? **Development**

### **Contributing**
1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

### **Build Configuration**
```cpp
// Debug build - full debugging information
#ifdef _DEBUG
    #define MIST_ENGINE_BUILD_TYPE "Debug"
#else
    #define MIST_ENGINE_BUILD_TYPE "Release"
#endif
```

### **Plugin Development**
```cpp
// Create custom plugin
class MyGameLogic : public Module {
    void Initialize() override {
        // Plugin initialization
    }
    
    void Update(float deltaTime) override {
        // Game logic updates
    }
};
```

---

## ?? **Academic Use**

MistEngine is designed for educational and research purposes:

- **?? MSc Dissertation Material** - Complete technical analysis
- **??? Software Engineering Patterns** - Advanced design pattern implementations
- **?? Research Applications** - Suitable for game engine research
- **?? Educational Resource** - Comprehensive learning material

See `docs/technical/MistEngine_Implementation_Analysis.md` for academic-quality documentation.

---

## ?? **Version History**

### **?? v0.3.0 - Release (Current)**
- ? Complete FPS game implementation
- ? AI-powered development assistance
- ? Professional game export system
- ? Hot-reloadable plugin architecture
- ? Advanced ECS with dual physics
- ? Comprehensive documentation

### **?? Previous Versions**
- `v0.2.1` - Pre-alpha with basic ECS
- `v0.1.0` - Initial prototype

---

## ?? **Showcase**

### **?? FPS Game Features**
- Multiple enemy AI types with different behaviors
- Physics-based projectile system
- Real-time health and scoring
- Multiple weapon types with unique characteristics
- Pause/resume game functionality

### **??? Development Tools**
- Real-time scene editing with transform manipulation
- GameObject creation and component management
- AI-powered code assistance and debugging
- Export system for standalone game distribution
- Hot-reloadable plugin development

### **??? Architecture Achievements**
- High-performance ECS with optimized memory layout
- Dual physics integration (Bullet + custom ECS)
- Advanced AI integration with context awareness
- Professional editor interface with ImGui
- Cross-platform foundation with Windows primary support

---

## ?? **Support & Community**

### **Documentation**
- ?? **Complete Guides** - See `docs/` folder
- ?? **AI Assistant** - Press F2 in engine
- ?? **GitHub Discussions** - Community support
- ?? **Issue Tracker** - Bug reports and feature requests

### **Learning Resources**
- ?? **Architecture Guide** - `docs/technical/MistEngine_Implementation_Analysis.md`
- ?? **Development Setup** - `docs/user-guide/AI_README.md`
- ?? **Game Development** - `docs/technical/IMPLEMENTATION_SUMMARY.md`
- ?? **Plugin Creation** - `docs/technical/MODULARITY_GUIDE.md`

---

## ?? **License**

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## ?? **Acknowledgments**

- **Bullet Physics** - Physics simulation
- **OpenGL** - Graphics rendering
- **GLFW** - Window management
- **GLM** - Mathematics library
- **ImGui** - Immediate mode GUI
- **Google Gemini** - AI assistance

---

<div align="center">

**?? Ready to build amazing games? Start with MistEngine! ??**

[?? Documentation](docs/README.md) | [?? AI Setup](docs/user-guide/AI_README.md) | [??? Architecture](docs/technical/MistEngine_Implementation_Analysis.md) | [?? Plugins](docs/technical/MODULARITY_GUIDE.md)

---

**Built with ?? using Modern C++14 • ECS Architecture • AI Integration**

*MistEngine 0.3.0 - Where Games Meet Innovation*

</div>
