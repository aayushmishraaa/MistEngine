# MistEngine v0.3.0 - Release Package

A modern C++14 game engine with AI integration and complete FPS game implementation.

## ?? Quick Start

### Running MistEngine
1. **Windows**: Double-click `Start-MistEngine.bat`
2. **PowerShell**: Run `.\Start-MistEngine.ps1`
3. **Manual**: Navigate to binaries folder and run `MistEngine.exe`

### Controls
- **WASD**: Move camera
- **Mouse**: Look around  
- **F1**: Toggle ImGui demo
- **F2**: Open AI assistant (requires API key)
- **F3**: Toggle Scene Editor mode

### FPS Game Mode
1. Click "?? START FPS GAME" in the UI
2. Use WASD to move, mouse to aim
3. Left-click to shoot enemies
4. Press R to reload
5. ESC to pause

## ?? AI Assistant Setup (Optional)

1. Press F2 to open AI configuration
2. Get API key from: https://aistudio.google.com/app/apikey
3. Enter your Gemini API key
4. Click "Save ^& Connect"
5. Start coding with AI assistance

## ?? Package Contents

- `MistEngine.exe` - Main engine executable
- `shaders/` - OpenGL shader files
- `assets/` - Game assets (if included)
- `Start-MistEngine.bat` - Windows launcher
- `Start-MistEngine.ps1` - PowerShell launcher

## ?? Features

### Complete FPS Game
- Intelligent enemy AI with different behaviors
- Multiple weapon types (Pistol, Rifle, Shotgun, Sniper)
- Physics-based combat and projectiles
- Health and scoring systems
- Pause/resume functionality

### AI-Powered Development
- Real-time AI assistant integrated into engine
- Google Gemini API integration
- Context-aware code suggestions
- Development help and debugging assistance

### Modern Engine Architecture  
- Entity-Component-System (ECS) design
- Dual physics systems (Bullet Physics + custom ECS)
- Advanced rendering pipeline with shadows
- Hot-reloadable plugin system
- Memory-safe RAII design patterns

## ?? System Requirements

- **OS**: Windows 10+ (x64)
- **Graphics**: OpenGL 3.3 compatible
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 200MB free space
- **Compiler**: Visual Studio 2019+ (for development)

## ?? Troubleshooting

### Game won't start
- Ensure you have latest graphics drivers
- Check Windows Event Viewer for error details
- Try running as administrator
- Verify all DLL files are present

### Missing DLL errors
- Install Visual C++ Redistributable 2019+
- Download from Microsoft official site
- Restart after installation

### AI Assistant issues
- Check internet connection
- Verify API key is correct
- Ensure you have Gemini API access
- Check console output for error details

## ?? Support

- **GitHub**: https://github.com/yourusername/MistEngine
- **Issues**: Report bugs via GitHub Issues
- **Discussions**: Ask questions in GitHub Discussions
- **AI Help**: Press F2 in the engine for development assistance

## ?? Credits

Built with love using:
- **OpenGL** for rendering
- **Bullet Physics** for collision detection  
- **Dear ImGui** for user interface
- **GLFW** for window management
- **GLM** for mathematics
- **Google Gemini** for AI integration

---

**MistEngine v0.3.0** - Ready for Production Use ??

*A Modern C++14 Game Engine with AI Integration*
