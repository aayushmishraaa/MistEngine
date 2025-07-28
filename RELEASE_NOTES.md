# ?? MistEngine Pre-Alpha Release Notes

## Version: v0.2.1-pre-alpha
**Release Date**: January 2025
**Build Status**: Pre-Alpha - Patch Release

---

## ?? What's New in v0.2.1 (Patch Release)

### ?? **Critical Fixes**
- **? Mouse Look System Fixed**: Complete overhaul of camera mouse controls
  - **Fixed unwanted camera spinning** when mouse look should be disabled
  - **Unity-like Scene Editor controls**: Right-click + drag for camera look
  - **Proper mouse lock**: Mouse movement only affects camera when intended
  - **UI clicking restored**: Fixed issues with clicking UI elements

### ?? **Input System Improvements**
- **? WASD Movement**: Fully working keyboard camera movement
- **? Polling-based Input**: Eliminated callback conflicts with ImGui
- **? Scene Editor Mode**: Professional Unity-like navigation controls
- **? Gameplay Mode**: Traditional FPS-style camera controls (F3 to toggle)

### ?? **Module System (Enhanced)**
- **? Dynamic Module Loading**: Automatic discovery of modules from `modules/` directory
- **? Module Distribution**: Proper module packaging in releases
- **? User-Friendly Module Installation**: Simply drop DLL files in modules/ folder
- **? Module Documentation**: Comprehensive guides for module development and usage

### ??? **Technical Improvements**
- **? Removed Legacy Input Conflicts**: Eliminated dual input processing
- **? InputManager Overhaul**: Complete rewrite of input handling system
- **? ImGui Compatibility**: Perfect integration with UI system
- **? Documentation Organization**: Moved development docs to organized structure

### ?? **Developer Experience**
- **? Clean Repository**: Development documentation properly organized
- **? Professional Controls**: Industry-standard editor navigation
- **? Stable Input**: Predictable and reliable input behavior
- **? Better Debugging**: Comprehensive input system documentation
- **? Module Templates**: Ready-to-use templates for creating custom modules

---

## ?? **New Controls (Fixed!)**

### Scene Editor Mode (Default)
- **WASD/QE**: Camera movement (works independently)
- **Right-click + drag**: Camera look around (Unity-style)
- **Mouse scroll**: Zoom in/out
- **F3**: Toggle to Gameplay mode
- **Free cursor**: Click UI elements without camera interference

### Gameplay Mode
- **WASD/QE**: Camera movement
- **Mouse movement**: Camera look (FPS-style)
- **Locked cursor**: Immersive gameplay experience
- **F3**: Return to Scene Editor mode

---

## ?? **Module System Guide**

### **For Users - Adding Modules**
1. **Download a module** (e.g., `PhysicsEnhancement.dll`)
2. **Copy to modules/ folder** in your MistEngine directory
3. **Restart MistEngine** ? Module automatically loads!
4. **Check console** for loading confirmation

### **For Developers - Creating Modules**
```cpp
// Use the module interface to extend the engine
class MyModule : public IComponentModule {
    bool Initialize() override { /* your code */ }
    void RegisterComponents(Coordinator* coordinator) override { /* add components */ }
};

DECLARE_MODULE(MyModule) // Export functions for loading
```

### **Module Types Supported**
- **Component Modules**: Add new ECS components
- **System Modules**: Add new ECS systems  
- **Renderer Extensions**: Add rendering features
- **Tool Modules**: Add editor tools
- **Script Modules**: Add scripting support

### **Module Directory Structure**
```
MistEngine/
??? MistEngine.exe
??? modules/                    ? Module directory
?   ??? ExampleModule.dll       ? Modules go here
?   ??? YourModule.dll
?   ??? README.md               ? Module usage guide
??? ... (other files)
```

---

## Version: v0.2.0-pre-alpha
**Release Date**: January 2025
**Build Status**: Pre-Alpha

---

## ?? What's New in v0.2.0

### ?? Core Engine Features
- **Entity Component System (ECS)**: Complete ECS implementation with coordinator pattern
- **OpenGL Rendering**: Modern OpenGL 3.3+ pipeline with shader support
- **Physics Integration**: Bullet Physics with rigid body simulation
- **Scene Management**: Hierarchical scene graph with transform management
- **Real-time Editor**: ImGui-based editor with dockable windows

### ?? AI Integration (Enhanced!)
- **Google Gemini AI**: Integrated AI assistant for development help
- **Interactive Chat**: F2 to open AI chat window
- **Code Suggestions**: Get help with engine features and implementations
- **Free Tier**: Uses Gemini's generous free API (15 requests/minute)
- **Quick Actions**: Pre-defined prompts for common development tasks
- **Improved UI**: Better chat interface and response handling

### ??? Development Tools
- **Hierarchy Window**: Manage scene entities
- **Inspector Window**: Edit component properties with intuitive controls
- **Console System**: Logging and debug output
- **Asset Creation**: Built-in primitives (Cube, Sphere, Plane)
- **Enhanced Documentation**: Comprehensive setup guides and tutorials

---

## ?? What You Can Do

### Build and Play
- ? Create and manipulate 3D objects
- ? Apply physics simulation (gravity, collisions)
- ? Navigate 3D scenes with professional camera controls
- ? Use AI assistant for development questions
- ? Edit object properties in real-time
- ? **NEW**: Extend functionality with custom modules

### Development
- ? Extend the ECS with custom components
- ? Create new rendering systems
- ? Add custom physics behaviors
- ? Integrate new AI providers
- ? Build upon the existing architecture
- ? **NEW**: Create and distribute modules as DLL files

### Modding and Extension
- ? **NEW**: Install community modules with simple file copy
- ? **NEW**: Create custom modules using provided templates
- ? **NEW**: Hot-reload modules during development
- ? **NEW**: Share modules with the community

---

## ?? Known Limitations (Pre-Alpha)

### Platform Support
- **Windows Only**: Currently requires Visual Studio and WinINet
- **No Linux/macOS**: Cross-platform support planned for future releases

### Features
- **No Audio System**: Sound integration not yet implemented
- **Basic Materials**: Advanced PBR materials coming soon
- **No Scripting**: Lua/C# scripting support planned
- **Limited Asset Pipeline**: More format support needed

### Performance
- **Optimized Release Builds**: Now available with better performance
- **Single-threaded**: Multi-threading planned for v0.3.0
- **No Culling**: View frustum culling not implemented

---

## ?? System Requirements

### Minimum
- **OS**: Windows 10/11
- **CPU**: Intel i3 / AMD equivalent
- **RAM**: 4GB
- **GPU**: OpenGL 3.3+ support
- **Storage**: 500MB

### Recommended  
- **OS**: Windows 10/11
- **CPU**: Intel i5 / AMD Ryzen 5
- **RAM**: 8GB+
- **GPU**: Dedicated graphics card
- **Storage**: 1GB

### Development Requirements
- **Visual Studio 2017+**
- **C++14 compiler**
- **Git** (for dependencies)

---

## ?? Getting Started

### Quick Setup
1. Download the pre-alpha release
2. Extract to desired folder
3. Run `Launch_MistEngine.bat`
4. Start creating!

### AI Assistant Setup
1. Get free API key: [Google AI Studio](https://aistudio.google.com/app/apikey)
2. In engine: **AI > Configure API Key**
3. Test connection and start chatting!

### Module Setup
1. **Check modules/ folder** for available modules
2. **Add new modules** by copying DLL files to modules/
3. **Restart engine** to load new modules
4. **Create your own** using provided templates

### First Steps
- Press **F2** to open AI assistant
- Use **GameObject menu** to create objects
- **Right-click** in Hierarchy for context menu
- Check **Window menu** for available panels
- **Explore modules/** folder for extensibility options

---

## ?? Known Issues

### Critical
- [ ] **Memory leaks** in mesh loading (being addressed)
- [ ] **Crash on exit** in debug builds occasionally

### Minor
- [ ] UI scaling issues on high-DPI displays
- [ ] Some keyboard shortcuts not working in AI window
- [ ] Physics debug rendering flickers

### Module-Specific
- [ ] **Hot reload** may occasionally require manual restart
- [ ] **Module dependencies** not fully validated
- [ ] **Module error reporting** could be more detailed

### AI-Specific
- [ ] Rate limit warnings not always shown
- [ ] Long responses may cause UI lag
- [ ] Connection timeout on first request sometimes

---

## ?? How to Contribute

We're actively seeking contributors! This pre-alpha release is perfect for:

### Developers
- **C++ programmers**: Core engine development
- **Graphics programmers**: Rendering improvements
- **AI enthusiasts**: More AI provider integrations
- **Cross-platform experts**: Linux/macOS ports
- **Module developers**: Create and share modules

### Non-Developers
- **Bug testers**: Help find and report issues
- **Documentation writers**: Improve guides and tutorials
- **3D artists**: Create test assets and examples
- **Game developers**: Provide feedback on usability
- **Module creators**: Build specialized modules for the community

### Priority Areas
1. **Cross-platform support** ??
2. **Performance optimization** ?
3. **More AI providers** ??
4. **Better asset pipeline** ??
5. **Audio system** ??
6. **Module ecosystem** ??

---

## ??? Development Roadmap

### v0.2.2 (Next Patch)
- [ ] Performance optimizations
- [ ] Additional input improvements
- [ ] Better error handling
- [ ] UI/UX enhancements
- [ ] Module system improvements

### v0.3.0 (Next Release)
- [ ] Linux support
- [ ] Basic audio system  
- [ ] More AI providers (OpenAI, Claude)
- [ ] Advanced scene editor features
- [ ] Module package manager

### v0.4.0 (Beta)
- [ ] Scripting support (Lua)
- [ ] Advanced materials
- [ ] Multi-threading
- [ ] Asset browser improvements
- [ ] Module marketplace

### v1.0.0 (Stable)
- [ ] Full cross-platform support
- [ ] Complete feature set
- [ ] Comprehensive documentation
- [ ] Example projects
- [ ] Robust module ecosystem

---

## ?? Special Thanks

### Contributors
- Core development team
- Early testers and feedback providers
- Open source community
- Module developers and early adopters

### Libraries & Tools
- **ImGui**: Excellent immediate mode GUI
- **Bullet Physics**: Robust physics simulation
- **Assimp**: Comprehensive model loading
- **Google Gemini**: AI integration capabilities

---

## ?? Support & Community

### Getting Help
- **GitHub Issues**: [Report bugs](https://github.com/aayushmishraaa/MistEngine/issues)
- **Discussions**: [Community forum](https://github.com/aayushmishraaa/MistEngine/discussions)
- **AI Assistant**: Built-in help system (F2)
- **Module Support**: Check modules/README.md for module-specific help

### Stay Updated
- **GitHub Releases**: Watch the repository
- **Development Blog**: Coming soon
- **Discord Community**: Coming soon
- **Module Showcase**: Coming soon

---

## ?? What's Next?

This patch release (v0.2.1) represents a significant improvement in user experience with completely fixed input controls and enhanced module system support. The engine now provides a professional, Unity-like development environment that's easily extensible.

### Immediate Goals
1. **Gather feedback** on the new input system and module experience
2. **Performance optimization** focus
3. **Cross-platform preparation** for v0.3.0
4. **Enhanced AI features** and more providers
5. **Growing module ecosystem** with community contributions

### Long-term Vision
MistEngine aims to be a **modern, AI-assisted, modular game engine** that makes game development more accessible and enjoyable for everyone, with a thriving ecosystem of community-created modules.

---

**Download now and experience the improved controls and module system! ???**

*Remember: This is pre-alpha software. Expect bugs, missing features, and breaking changes. But also expect rapid development, community involvement, and an exciting modular architecture!*