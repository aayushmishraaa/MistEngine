# ?? MistEngine

**A Modern C++14 Game Engine with Integrated AI Development Assistant**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C%2B%2B-14-blue.svg)](https://en.cppreference.com/w/cpp/14)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3-green.svg)](https://www.opengl.org/)
[![Status](https://img.shields.io/badge/Status-Pre--Alpha-red.svg)](https://github.com/aayushmishraaa/MistEngine)

> **Revolutionary game engine that integrates AI assistance directly into the development workflow, featuring a sophisticated Entity Component System and modular plugin architecture.**

---

## ? Key Features

### ?? **Core Engine**
- **Entity Component System (ECS)** - High-performance data-oriented architecture
- **Modern OpenGL Rendering** - PBR-ready pipeline with shadow mapping
- **Bullet Physics Integration** - Realistic physics simulation with dual legacy/ECS systems
- **Professional Scene Editor** - Unity-like interface with ImGui integration
- **Modular Plugin System** - Runtime extensibility through dynamic module loading

### ?? **AI-Powered Development**
- **Integrated AI Assistant** - Real-time coding help via Google Gemini API
- **Context-Aware Suggestions** - AI understands your engine architecture
- **Code Analysis & Review** - Get instant feedback on your implementations
- **Feature Brainstorming** - AI helps design new engine features

### ? **Advanced Systems**
- **Smart Input Management** - Context-aware input handling (Scene Editor / Gameplay modes)
- **Hot Module Reloading** - Update engine functionality without restart
- **Cross-Platform Foundation** - Designed for Windows with Linux/macOS extensibility
- **Professional Workflow** - Industry-standard development tools and interfaces

---

## ?? Quick Start

### Prerequisites
- **Visual Studio 2017+** (Windows)
- **Git** for version control
- **Basic C++14 knowledge**

### ?? Installation & Setup

1. **Clone the repository**
   ```bash
   git clone https://github.com/aayushmishraaa/MistEngine.git
   cd MistEngine
   ```

2. **Open in Visual Studio**
   ```bash
   # Open the solution file
   start MistEngine.sln
   ```

3. **Build & Run**
   - Press `F5` to build and run
   - The engine should compile and run immediately (all dependencies included)

4. **First Launch Setup**
   - Press `F2` to open AI Assistant
   - Configure your Gemini API key (optional but recommended)
   - Start creating your first 3D scene!

---

## ?? Controls & Interface

### ??? **Scene Editor Mode** (Default)
- **WASD/QE** - Camera movement (fly around the scene)
- **Right-click + Drag** - Mouse look (Unity-style camera control)
- **Mouse Scroll** - Zoom in/out
- **F3** - Toggle to Gameplay mode
- **F1** - Toggle ImGui demo window
- **F2** - Open/Close AI Assistant

### ?? **Gameplay Mode**
- **WASD/QE** - Camera movement
- **Mouse Movement** - Free look (FPS-style)
- **F3** - Return to Scene Editor mode

### ??? **Development Tools**
- **GameObject Menu** - Create cubes, planes, spheres
- **Hierarchy Window** - Manage scene objects
- **Inspector Window** - Edit object properties in real-time
- **Console Window** - Debug output and engine messages
- **AI Assistant** - Get help with coding and engine features

---

## ??? Architecture Overview

MistEngine demonstrates advanced software engineering through its layered architecture:

```
???????????????????????????????????????????
?        Application Layer                 ?  ? Main Engine Loop & Game Logic
???????????????????????????????????????????
?        Systems Layer                     ?  ? ECS Systems, Module Manager
???????????????????????????????????????????
?        Component Layer                   ?  ? ECS Components, Data Structures  
???????????????????????????????????????????
?        Subsystems Layer                  ?  ? Renderer, Physics, Input, UI, AI
???????????????????????????????????????????
?        Foundation Layer                  ?  ? Math, Memory, Platform Abstraction
???????????????????????????????????????????
```

### ?? **Design Principles**
- **Data-Oriented Design** - ECS for cache-friendly performance
- **Composition over Inheritance** - Flexible entity creation
- **Dependency Injection** - Clean, testable system architecture
- **RAII Resource Management** - Automatic memory and OpenGL resource cleanup
- **Plugin Architecture** - Extensible through dynamic modules

---

## ?? AI Integration Features

### **Real-time Development Assistance**
```cpp
// Ask AI about engine architecture
"How should I implement a new component type?"

// Get code suggestions  
"Help me optimize this rendering loop"

// Debug assistance
"Why is my physics simulation unstable?"
```

### **Supported AI Providers**
- ? **Google Gemini** - Excellent for code analysis and architecture advice
- ?? **OpenAI GPT** (Coming Soon) - Alternative provider support
- ?? **Anthropic Claude** (Planned) - Additional provider option

### **AI Features**
- **Conversation Memory** - Maintains context across development sessions
- **Code Analysis** - Understands C++ and game engine patterns
- **Architecture Guidance** - Helps design new systems and features
- **Debugging Support** - Assists with troubleshooting complex issues

---

## ?? Module System

### **For Users - Adding Modules**
1. Download a module (`.dll` file)
2. Copy to `modules/` folder
3. Restart MistEngine ? Module loads automatically!

### **For Developers - Creating Modules**
```cpp
// Example: Custom Physics Enhancement Module
class PhysicsEnhancementModule : public IModule {
public:
    bool Initialize() override {
        // Initialize your custom physics features
        return true;
    }
    
    void Update(float deltaTime) override {
        // Update your custom systems
    }
    
    ModuleInfo GetInfo() const override {
        return {"PhysicsEnhancement", "1.0", "Your Name", "Enhanced physics features"};
    }
};

// Register the module
DECLARE_MODULE(PhysicsEnhancementModule)
```

---

## ??? Technical Specifications

### **Core Technologies**
- **Language**: C++14 (maximum compatibility)
- **Graphics**: OpenGL 3.3 Core Profile
- **Physics**: Bullet Physics 3.x
- **GUI**: Dear ImGui (immediate mode)
- **Windowing**: GLFW 3
- **Mathematics**: GLM (OpenGL Mathematics)
- **AI Integration**: Google Gemini API
- **HTTP Client**: Custom WinINet wrapper (Windows)

### **Performance Features**
- **Data-Oriented ECS** - Cache-friendly component storage
- **Batch Rendering** - Efficient draw call management
- **Smart Culling** - Frustum and occlusion culling
- **Hot Module Reloading** - Runtime code updates
- **Multithreaded Systems** - Parallel system execution (planned)

### **Development Features**
- **Live Component Editing** - Real-time property modification
- **Scene Serialization** - Save/load complete scenes
- **Asset Hot Reloading** - Update assets without restart
- **Comprehensive Logging** - Detailed debug information
- **Memory Profiling** - Track resource usage

---

## ?? Documentation

### **?? Getting Started**
- [Contributing Guide](CONTRIBUTING.md) - How to contribute to MistEngine
- [Release Notes](RELEASE_NOTES.md) - Latest changes and updates
- [AI Setup Guide](docs/user-guide/AI_README.md) - Configure AI assistant

### **?? Technical Documentation**
- [Implementation Analysis](docs/technical/MistEngine_Implementation_Analysis.md) - Detailed system architecture
- [Module Development Guide](docs/technical/MODULARITY_GUIDE.md) - Create custom plugins
- [ECS Architecture](docs/technical/IMPLEMENTATION_SUMMARY.md) - Entity Component System details

### **?? Development Docs**
- [Input System](docs/development/FINAL_MOUSE_FIX.md) - Mouse and keyboard handling
- [Scene Editor](docs/development/SCENE_EDITOR_FIXES.md) - Editor implementation details
- [Physics Integration](docs/development/) - Physics system documentation

---

## ?? Showcase

### **What You Can Build**
- **3D Games** - Full-featured game development
- **Simulations** - Physics-based simulations and demos
- **Prototypes** - Rapid game concept prototyping
- **Learning Projects** - Advanced C++ and game engine concepts
- **Engine Modules** - Custom functionality plugins

### **Perfect For**
- ?? **Students** learning game engine architecture
- ?? **Researchers** exploring ECS patterns and AI integration
- ?? **Indie Developers** building games with modern tools
- ????? **Professional Developers** prototyping new ideas
- ?? **Academic Projects** demonstrating software engineering principles

---

## ?? Contributing

We welcome contributions! MistEngine is in active development and there are many ways to help:

### **?? High Priority**
- **Cross-platform Support** - Linux and macOS compatibility
- **Additional AI Providers** - OpenAI, Claude integration
- **Performance Optimization** - Multi-threading, rendering improvements
- **Advanced Editor Features** - Asset browser, scene serialization

### **?? Good First Issues**
- UI improvements and keyboard shortcuts
- Additional primitive shapes (cylinders, capsules)
- Code documentation and comments
- Bug fixes and stability improvements

**See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines.**

---

## ?? License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

**You are free to:**
- ? Use MistEngine for personal and commercial projects
- ? Modify and extend the engine for your needs
- ? Distribute your modifications
- ? Create plugins and modules for the engine

---

## ?? Acknowledgments

### **Dependencies**
- [OpenGL](https://www.opengl.org/) - Graphics API
- [GLFW](https://www.glfw.org/) - Cross-platform windowing
- [GLAD](https://glad.dav1d.de/) - OpenGL function loader
- [GLM](https://glm.g-truc.net/) - Mathematics library
- [Bullet Physics](https://bulletphysics.org/) - Physics simulation
- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI
- [Google Gemini API](https://ai.google.dev/) - AI integration

### **Inspiration**
- **Unity** - Scene editor interface and workflow design
- **Unreal Engine** - Professional development tool inspiration
- **EnTT** - Modern ECS architecture patterns
- **Godot** - Open-source game engine architecture

---

## ?? Contact & Community

- **GitHub Issues** - [Report bugs and request features](https://github.com/aayushmishraaa/MistEngine/issues)
- **GitHub Discussions** - [Ask questions and share ideas](https://github.com/aayushmishraaa/MistEngine/discussions)
- **AI Assistant** - Press F2 in the engine for real-time help!

---

<div align="center">

**?? Ready to build the future of game development?**

[? Star the Project](https://github.com/aayushmishraaa/MistEngine) | [?? Fork & Contribute](https://github.com/aayushmishraaa/MistEngine/fork) | [?? Read the Docs](docs/)

**MistEngine v0.2.1-pre-alpha** | Made with ?? and AI assistance

</div>