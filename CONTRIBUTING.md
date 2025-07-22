# ?? Contributing to MistEngine

Thank you for your interest in contributing to MistEngine! This project is in pre-alpha stage and we welcome all types of contributions.

---

## ?? Quick Start for Contributors

### 1. Set Up Development Environment

**Prerequisites:**
- Visual Studio 2017+ (Windows)
- Git
- Basic C++14 knowledge

**Setup Steps:**
1. Fork the repository on GitHub
2. Clone your fork:
   ```bash
   git clone https://github.com/aayushmishraaa/MistEngine.git
   cd MistEngine
   ```
3. Open `MistEngine.sln` in Visual Studio
4. Build the solution (should work out of the box)
5. Run to ensure everything works

### 2. Create Your First Contribution

1. Create a new branch:
   ```bash
   git checkout -b feature/your-amazing-feature
   ```
2. Make your changes
3. Test thoroughly
4. Commit and push:
   ```bash
   git commit -m "Add: your amazing feature"
   git push origin feature/your-amazing-feature
   ```
5. Open a Pull Request

---

## ?? Ways to Contribute

### ?? Bug Fixes
- **High Impact**: Crashes, memory leaks, major functionality issues
- **Medium Impact**: UI glitches, minor functionality problems
- **Low Impact**: Polish, small improvements

### ? New Features
- **Core Engine**: ECS improvements, rendering features, physics enhancements
- **AI Integration**: New AI providers, better chat interface, AI-powered tools
- **Editor Tools**: New windows, improved workflows, debugging tools
- **Cross-platform**: Linux/macOS support (high priority!)

### ?? Documentation
- **Code Documentation**: Comments, API docs
- **User Guides**: Tutorials, how-to guides
- **Developer Docs**: Architecture explanations, contribution guides

### ?? Testing
- **Bug Reports**: Find and report issues
- **Feature Testing**: Test new functionality
- **Performance Testing**: Profile and optimize

---

## ??? Development Guidelines

### Code Style

**C++ Standards:**
- Use **C++14** features (project requirement)
- Follow **PascalCase** for classes/structs: `PhysicsComponent`
- Follow **camelCase** for functions/variables: `updatePhysics()`
- Use **m_** prefix for member variables: `m_entityCount`
- Use descriptive names: `calculateLightingMatrix()` not `calc()`

**File Organization:**
```
include/          # Header files (.h)
src/              # Implementation files (.cpp)
shaders/          # GLSL shader files
AI/               # AI-related code (separate namespace)
ECS/              # Entity Component System
```

**Example Code Style:**
```cpp
class RenderComponent {
private:
    Mesh* m_mesh;
    bool m_isVisible;
    
public:
    void SetMesh(Mesh* mesh) { m_mesh = mesh; }
    bool IsVisible() const { return m_isVisible; }
    
    void UpdateRenderState(float deltaTime) {
        // Implementation here
    }
};
```

### Architecture Principles

**ECS Pattern:**
- **Components**: Pure data, no logic
- **Systems**: Pure logic, operate on components  
- **Entities**: Just IDs, managed by coordinator

**RAII and Memory Management:**
- Use smart pointers where appropriate
- Manual memory management for performance-critical parts
- Always clean up resources in destructors

**AI Integration:**
- Keep AI code in separate `AI/` namespace
- Use provider pattern for different AI services
- Make AI features optional (fail gracefully if no API key)

### Commit Guidelines

**Format:**
```
Type: Brief description (50 chars max)

Longer explanation if needed (wrap at 72 chars)
- What changed
- Why it changed  
- Any breaking changes
```

**Types:**
- **Add**: New features
- **Fix**: Bug fixes
- **Update**: Modifications to existing features
- **Remove**: Removing features/code
- **Refactor**: Code reorganization
- **Docs**: Documentation changes

**Examples:**
```bash
Add: Gemini AI provider integration

- Implement GeminiProvider class with HTTP client
- Add configuration management for API keys  
- Integrate with existing AIManager
- Closes #42

Fix: Memory leak in mesh loading system

- Properly delete vertex buffers in Mesh destructor
- Add RAII wrapper for OpenGL resources
- Fixes crash on scene reload
```

---

## ?? Testing Guidelines

### Before Submitting PR

**Functionality Testing:**
- [ ] Build succeeds in Debug and Release
- [ ] No new compiler warnings
- [ ] Core features still work (create objects, AI chat, physics)
- [ ] No obvious memory leaks (check Task Manager)

**AI Testing (if applicable):**
- [ ] Test with valid API key
- [ ] Test with invalid API key (should fail gracefully)
- [ ] Test rate limiting behavior
- [ ] Test long conversations

**Integration Testing:**
- [ ] Test with other systems (physics + rendering, ECS + AI, etc.)
- [ ] Test edge cases and error conditions
- [ ] Verify backward compatibility

### Performance Testing

**For Performance Changes:**
- Profile before and after changes
- Document performance impact in PR
- Consider memory usage and frame rate impact

**Tools:**
- Visual Studio Diagnostics
- Application Verifier (for memory issues)
- GPU profilers (for rendering changes)

---

## ?? Priority Areas

### ?? High Priority (Help Wanted!)

1. **Cross-platform Support**
   - Replace WinINet HTTP client with cross-platform alternative
   - CMake build system
   - Linux/macOS testing

2. **Performance Optimization**
   - Multi-threading for systems
   - Better memory management
   - Rendering optimizations

3. **AI Provider Expansion**
   - OpenAI GPT integration
   - Anthropic Claude support
   - Local AI model support

4. **Core Engine Features**
   - Audio system
   - Advanced materials
   - Animation system

### ?? Medium Priority

1. **Editor Improvements**
   - Asset browser
   - Scene serialization
   - Better debugging tools

2. **Documentation**
   - API documentation
   - Video tutorials
   - Architecture guides

### ?? Good First Issues

Perfect for new contributors:

1. **UI Improvements**
   - Add keyboard shortcuts
   - Improve tooltips
   - Fix layout issues

2. **Code Quality**
   - Add comments to unclear code
   - Fix compiler warnings
   - Improve error messages

3. **Small Features**
   - Add new primitive shapes
   - Improve console output
   - Add configuration options

---

## ?? AI-Specific Contributions

### New AI Providers

To add a new AI provider (e.g., OpenAI, Claude):

1. **Create provider class:**
   ```cpp
   class OpenAIProvider : public AIProvider {
       // Implement required methods
   };
   ```

2. **Update AIManager:**
   - Add provider registration
   - Update configuration handling

3. **Test thoroughly:**
   - Different API endpoints
   - Error handling
   - Rate limiting

4. **Document setup:**
   - How to get API key
   - Configuration steps
   - Usage examples

### AI Feature Ideas

- **Code Analysis**: AI reviews user code for suggestions
- **Asset Generation**: AI creates textures/models
- **Debugging Assistant**: AI helps diagnose issues
- **Documentation**: AI explains engine systems

---

## ?? What NOT to Contribute

### Avoid These Changes

1. **Breaking API Changes** (without discussion)
2. **Major Architecture Changes** (discuss first in issues)
3. **Platform-Specific Code** (without cross-platform alternatives)
4. **Large Dependencies** (keep it lightweight)
5. **Non-Free Assets** (use CC0/MIT licensed assets only)

### Code Standards to Avoid

1. **Global Variables** (use singletons or dependency injection)
2. **Magic Numbers** (use named constants)
3. **Deep Inheritance** (prefer composition)
4. **Platform-Specific Headers** in common code

---

## ?? Communication

### Before Starting Major Work

**Please discuss:**
- Large features or architecture changes
- New dependencies
- Breaking changes
- Significant performance modifications

**Where to Discuss:**
- **GitHub Issues**: For bugs and small features
- **GitHub Discussions**: For design decisions and questions
- **Pull Requests**: For code review and specific implementation

### Getting Help

**Stuck on something?**
1. Check existing issues and discussions
2. Use the AI assistant (F2) for coding questions
3. Ask in GitHub Discussions
4. Look at existing code for patterns

**Response Times:**
- We aim to respond to issues within 48 hours
- Pull requests reviewed within a week
- Complex discussions may take longer

---

## ?? Recognition

### Contributors

All contributors are recognized:
- **README.md**: Major contributors listed
- **Release Notes**: Contributions acknowledged
- **Commit History**: Your commits are permanent record

### Special Recognition

- **First-time Contributors**: Special thanks in release notes
- **Major Features**: Highlighted in project documentation
- **Bug Hunters**: Recognition for finding critical issues

---

## ?? Legal

### Licensing

By contributing to MistEngine:
- You agree your contributions will be licensed under MIT License
- You confirm you have the right to submit the code
- You retain copyright to your contributions

### Code of Conduct

**Be Respectful:**
- Welcome newcomers and help them learn
- Provide constructive feedback
- Focus on the code, not the person
- Respect different opinions and approaches

**Be Professional:**
- Use clear, professional language in issues/PRs
- Test your code before submitting
- Write meaningful commit messages
- Follow the established patterns

---

## ?? Getting Started Checklist

Ready to contribute? Here's your checklist:

**Setup:**
- [ ] Fork and clone the repository
- [ ] Build the project successfully
- [ ] Run the engine and test basic functionality
- [ ] Read through the codebase to understand structure

**First Contribution:**
- [ ] Pick a "good first issue" or small bug
- [ ] Create a new branch for your work
- [ ] Make your changes following code style guidelines
- [ ] Test thoroughly
- [ ] Submit a pull request with clear description

**Ongoing:**
- [ ] Join GitHub Discussions for project updates
- [ ] Help other contributors with questions
- [ ] Suggest improvements and new features
- [ ] Share your experience building with MistEngine

---

**Welcome to the MistEngine community! ???**

*Let's build something amazing together!*