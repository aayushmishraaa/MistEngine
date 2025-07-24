


# 🎮 MistEngine v0.2.0 Pre-Alpha Release
**The second pre-alpha release of MistEngine - A Modern C++ Game Engine with AI Integration!**

---

## 🎯 What is MistEngine?

MistEngine is a modern, developer-friendly game engine built in C++ with integrated AI assistance. This v0.2.0 pre-alpha release showcases enhanced features and improved stability.

### ✨ Key Features in This Release

- **🏗️ Entity Component System**: Clean, modern ECS architecture
- **🎨 OpenGL Rendering**: Modern OpenGL 3.3+ pipeline with real-time lighting
- **⚡ Physics Simulation**: Bullet Physics integration with real-time physics
- **🤖 AI Assistant**: Enhanced Google Gemini AI for development help (F2)
- **🛠️ Live Editor**: ImGui-based editor with hierarchy, inspector, and console
- **🎮 Interactive Scene**: Create cubes, spheres, planes with physics
- **📚 Comprehensive Documentation**: Improved setup guides and tutorials

### 🤖 AI Integration Highlights

- **Press F2** to open the AI assistant
- **Free tier support** with Google Gemini (15 requests/minute)
- **Enhanced UI** with better response handling
- **Context-aware help** for game development
- **Code suggestions** and architecture advice
- **Quick setup** with built-in configuration dialog

---

## 📥 Download & Installation

### System Requirements
- **OS**: Windows 10/11 (64-bit)
- **Graphics**: OpenGL 3.3+ support
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 500MB

### Quick Start
1. **Download** the ZIP file below
2. **Extract** to your desired folder
3. **Run** `Launch_MistEngine.bat`
4. **Optional**: Set up AI assistant (see `QUICK_START.md`)

### AI Setup (Optional but Recommended!)
1. Get free API key: [Google AI Studio](https://aistudio.google.com/app/apikey)
2. In engine: **AI Menu → Configure API Key**
3. Test connection and start chatting!

---

## 🎮 What You Can Do

### Core Features
- ✅ Create 3D primitives (Cube, Sphere, Plane)
- ✅ Apply physics simulation with gravity and collisions
- ✅ Navigate 3D scenes with WASD + mouse
- ✅ Edit object properties in real-time
- ✅ Chat with AI assistant about development

### AI Assistant Features
- 🤖 **General development help**: Ask about C++, OpenGL, game design
- 💡 **Feature suggestions**: Get ideas for your projects
- 🔧 **Code assistance**: Get help with implementation
- 🎯 **Quick actions**: Pre-defined prompts for common tasks

### Controls
- **WASD**: Camera movement
- **Mouse**: Look around (when focused)
- **F2**: Open AI assistant
- **Right-click**: Context menus
- **GameObject menu**: Create objects
- **Window menu**: Show/hide panels

---

## 🆕 What's New in v0.2.0

### 🔧 **Improvements**
- **Enhanced Release Builds**: Better performance and stability
- **Improved AI Integration**: Better error handling and user experience
- **Documentation Overhaul**: Comprehensive guides for setup and development
- **Build System**: Streamlined PowerShell build scripts
- **Bug Fixes**: Resolved memory leaks and stability issues

### 🎨 **UI/UX Enhancements**
- Better AI chat interface
- Improved error messages
- Enhanced console output
- More intuitive controls

---

## 🚧 Pre-Alpha Limitations

**Remember**: This is a pre-alpha release! Expect:

### Known Limitations
- **Windows only** (Linux/macOS planned for v0.3.0)
- **No audio system** yet
- **Basic materials** only
- **Single-threaded** (multi-threading planned)
- **Occasional crashes** on exit

### What's Missing
- Advanced rendering features (PBR, post-processing)
- Scripting support (Lua integration planned)
- Asset import pipeline
- Animation system
- Networking capabilities

---

## 🐛 Found a Bug?

This is pre-alpha software, so bugs are expected!

**Please report:**
- **Crashes** or hangs
- **AI assistant issues**
- **Performance problems**
- **UI/UX issues**

**How to report:**
1. [Open a GitHub Issue](https://github.com/aayushmishraaa/MistEngine/issues)
2. Include your system specs
3. Describe steps to reproduce
4. Attach screenshots if helpful

---

## 🤝 Want to Contribute?

We're actively looking for contributors! This is a great time to get involved.

### Ways to Help
- **🐛 Bug testing**: Find and report issues
- **💻 Code contributions**: C++, graphics, AI features
- **📚 Documentation**: Improve guides and tutorials
- **🎨 Assets**: Create example models/textures
- **🌐 Cross-platform**: Help with Linux/macOS support

### Easy First Contributions
- Fix UI scaling issues
- Add keyboard shortcuts
- Improve error messages
- Add more primitive shapes
- Enhance AI prompts

**See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines**

---

## 🗺️ Roadmap

### v0.3.0 (Next Release - Q2 2025)
- [ ] **Linux support** 🐧
- [ ] **Basic audio system** 🔊
- [ ] **OpenAI integration** alongside Gemini
- [ ] **Performance optimizations** ⚡
- [ ] **Better error handling**

### v0.4.0 (Beta - Q3 2025)
- [ ] **Scripting support** (Lua)
- [ ] **Advanced materials**
- [ ] **Animation system**
- [ ] **Asset browser**

### v1.0.0 (Stable - 2026)
- [ ] **Full cross-platform support**
- [ ] **Complete feature set**
- [ ] **Production-ready performance**
- [ ] **Comprehensive documentation**

---

## 📊 Technical Details

### Architecture
- **Language**: C++14
- **Graphics**: OpenGL 3.3+
- **Physics**: Bullet Physics 3.x
- **UI**: Dear ImGui
- **AI**: Google Gemini API
- **Platform**: Windows (Visual Studio)

### Dependencies Included
- GLFW (window management)
- GLAD (OpenGL loader)  
- GLM (mathematics)
- Assimp (model loading)
- stb_image (texture loading)
- Bullet Physics (physics simulation)
- ImGui (user interface)

### Build Requirements (for developers)
- Visual Studio 2017+
- Windows 10/11
- Git

---

## 🙏 Acknowledgments

### Special Thanks
- **ImGui**: For the excellent immediate mode GUI
- **Bullet Physics**: For robust physics simulation  
- **Google Gemini**: For making AI integration accessible
- **OpenGL Community**: For graphics programming resources
- **Early testers**: For valuable feedback and bug reports

### Libraries Used
- **GLFW**, **GLAD**, **GLM**: Graphics foundation
- **Assimp**: 3D model loading
- **stb libraries**: Image and utility functions
- **Bullet Physics**: Physics simulation
- **Dear ImGui**: User interface

---

## 📞 Support & Community

### Getting Help

- **🐛 Issues**: [Report bugs here](https://github.com/aayushmishraaa/MistEngine/issues)
- **💬 Discussions**: [Community forum](https://github.com/aayushmishraaa/MistEngine/discussions)
- **🤖 AI Assistant**: Built-in help (press F2)
- **📚 Documentation**: See `docs/` folder in download


### Stay Updated
- **⭐ Star this repository** for updates
- **👁️ Watch releases** to get notified
- **🍴 Fork** to start contributing

---

## 📄 License

MistEngine is licensed under the **MIT License** - see [LICENSE](LICENSE) for details.

**TL;DR**: Free to use, modify, and distribute. Give credit where credit is due!

---

## 🚀 What's Next?

This v0.2.0 pre-alpha represents **significant progress** in MistEngine development. We're excited to see what the community thinks and builds!

### Immediate Goals
1. **Gather feedback** from early adopters
2. **Fix critical bugs** found by users  
3. **Improve documentation** based on questions
4. **Plan v0.3.0** features with community input

### Long-term Vision
MistEngine aims to be the **most developer-friendly game engine** with:
- **AI-assisted development** workflow
- **Modern C++ architecture**
- **Cross-platform support**
- **Active community** involvement

---

## 📦 Release Files

### Download Options

**MistEngine-v0.2.0-pre-alpha-win64.zip** ⬇️
- Complete pre-alpha build for Windows 64-bit
- Includes engine, documentation, and examples
- Size: ~40 MB
- **Recommended for most users**

### What's Included
```
MistEngine-v0.2.0-pre-alpha-win64/
├── bin/                     # Executable and DLLs
├── shaders/                 # GLSL shaders
├── models/                  # Example 3D models
├── textures/               # Example textures
├── docs/                   # Documentation
├── Launch_MistEngine.bat   # Easy launcher
├── QUICK_START.md          # Quick start guide
└── ai_config.example.json  # AI configuration template
```

---

**Download now and start building! 🎮✨**

**Join our community and help shape the future of MistEngine!**

*Remember: This is pre-alpha software. Expect bugs, but also expect rapid development and lots of community involvement!*
