

# MistEngine - 3D Engine

MistEngine is a simple 3D engine that demonstrates basic lighting and shadow rendering using OpenGL. It features a sun-like directional light that casts shadows on a plane below a rotating cube.

## Features
- **Directional Light**: A sun-like light source that shines from a specific direction.
- **Texture Loading**: Added a class implementaion of loading textures onto objects.
- **Real-Time Shadows**: Real-time shadow rendering.
- **Camera Controls**: Move around the scene using the keyboard (`W`, `A`, `S`, `D`) and mouse.

## Prerequisites
Before running the project, ensure you have the following installed:
- **CMake** (version 3.10 or higher)
- **OpenGL** (version 3.3 or higher)
- **GLFW** (for window and input management)
- **GLAD** (for OpenGL function loading)
- **GLM** (for mathematics operations)

## Setup
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/aayushmishraaa/MistEngine.git
   cd MistEngine
   ```

2. **Install Dependencies**:
   - Install GLFW:
     ```bash
     sudo apt-get install libglfw3-dev
     ```
   - Install GLM:
     ```bash
     sudo apt-get install libglm-dev
     ```

3. **Build the Project**:
   - Create a `build` directory and compile the project:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```

4. **Run the Engine**:
   - After building, run the executable:
     ```bash
     ./MistEngine
     ```

## Controls
- **Movement**:
  - `W`: Move forward
  - `A`: Move left
  - `S`: Move backward
  - `D`: Move right
- **Mouse**: Look around
- **Scroll Wheel**: Zoom in/out
- **Escape**: Close the window




## Lighting
- **Directional Light**:
  - The light direction is defined as `(-0.2f, -1.0f, -0.3f)`.
  - The light color is white (`1.0f, 1.0f, 1.0f`).
- **Shadows**:
  - Shadows are cast on the plane below the cube based on the light direction.

## Future Improvements
- **Shadow Mapping**: Implement shadow mapping for more realistic shadows.
- **Textures**: Add texture support for the plane and cube. **IMPLEMENTED**
- **Multiple Lights**: Add support for multiple light sources (point lights, spotlights).

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---
