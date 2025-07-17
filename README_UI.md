# MistEngine with ImGui Integration

## Building the Project

### Prerequisites
- Visual Studio 2019 or later
- vcpkg package manager
- CMake (optional, for alternative build system)

### Setup Instructions

1. **Install vcpkg dependencies:**
   ```bash
   vcpkg install imgui[opengl3-binding,glfw-binding,docking-experimental]:x64-windows
   ```

2. **If using existing project files:**
   - Add ImGui include directories to your project settings
   - Link against ImGui libraries
   - Add the UI source files to your project

3. **If using CMake:**
   ```bash
   mkdir build
   cd build
   cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ..
   cmake --build .
   ```

## UI Features

The integrated ImGui editor provides:

### Main Panels
- **Scene Hierarchy**: View and manage all entities in the scene
- **Inspector**: Edit properties of selected entities and their components
- **Asset Browser**: Browse and manage project assets
- **Console**: View engine logs and messages
- **Viewport**: Scene rendering with focus/hover detection
- **Stats**: Display performance metrics

### Entity Management
- Create, delete, and duplicate entities
- Add/remove components (Transform, Render, Physics)
- Edit component properties in real-time
- Scene graph navigation

### Component Editing
- **Transform Component**: Position, rotation, scale with color-coded xyz controls
- **Render Component**: Visibility toggle and renderable asset info
- **Physics Component**: Physics body settings and mass display

### Controls
- **TAB**: Toggle between cursor modes (UI/Camera control)
- **WASD**: Camera movement (when viewport focused)
- **Mouse**: Camera look (when viewport focused and hovered)
- **IJKL + Space**: Physics object control

### Modern UI Design
- Dark theme similar to Unity/Unreal Engine
- Dockable panels with full docking support
- Color-coded property controls
- Context menus for entity operations
- Real-time console logging

## Usage

1. **Launch the application**: The editor UI will initialize automatically
2. **Navigate the scene**: Click on the viewport to focus it, then use WASD to move
3. **Select entities**: Click on entities in the Scene Hierarchy
4. **Edit properties**: Use the Inspector panel to modify component values
5. **Create objects**: Use GameObject menu or toolbar buttons
6. **Toggle cursor**: Press TAB to switch between UI and camera control modes

## Code Structure

- `include/UI/EditorUI.h` - Main UI class declaration
- `src/UI/EditorUI.cpp` - UI implementation with all panels
- `src/Renderer.cpp` - Updated with UI integration
- `src/MistEngine.cpp` - Main loop with UI setup

The UI system is designed to be non-intrusive and can be easily disabled if needed.