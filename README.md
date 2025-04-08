

# MistEngine - 3D Engine

MistEngine is a 3D engine that demonstrates advanced rendering capabilities using OpenGL. It features dynamic lighting, shadow rendering, 3D model loading, and texture management for creating immersive 3D scenes.

## Features
- **3D Model Loading**: Support for loading complex 3D models in various formats (OBJ, FBX, etc.)
- **Texture Management**: Full texture support including diffuse, specular, normal, and ambient occlusion maps
- **Directional Light**: A sun-like light source that shines from a specific direction
- **Shadow System**: Dynamic shadow casting and receiving for all objects in the scene
- **Camera Controls**: Move around the scene using the keyboard (`W`, `A`, `S`, `D`) and mouse

## Prerequisites
Before running the project, ensure you have the following installed:
- **CMake** (version 3.10 or higher)
- **OpenGL** (version 3.3 or higher)
- **GLFW** (for window and input management)
- **GLAD** (for OpenGL function loading)
- **GLM** (for mathematics operations)
- **Assimp** (for 3D model loading)

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
   - Install Assimp:
     ```bash
     sudo apt-get install libassimp-dev
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

## File Structure
- **`src/`**: Contains the source code for the engine
  - `MistEngine.cpp`: Main application logic
  - `Shader.cpp`: Shader loading and compilation
  - `Camera.cpp`: Camera movement and view matrix calculation
  - `Model.cpp`: 3D model loading and rendering
  - `Mesh.cpp`: Mesh data structure and rendering
  - `Texture.cpp`: Texture loading and management
- **`include/`**: Contains header files
  - `Shader.h`: Shader class declaration
  - `Camera.h`: Camera class declaration
  - `Model.h`: Model class declaration
  - `Mesh.h`: Mesh class declaration
  - `Texture.h`: Texture class declaration
- **`shaders/`**: Contains GLSL shader files
  - `vertex.glsl`: Vertex shader
  - `fragment.glsl`: Fragment shader
  - `depth_vertex.glsl`: Shadow mapping vertex shader
  - `depth_fragment.glsl`: Shadow mapping fragment shader
- **`assets/`**: Contains 3D models and textures
- **`models/`**: Contains 3D model files
- **`textures/`**: Contains texture files

## Asset Support
### Supported 3D Model Formats
- Wavefront OBJ (.obj)
- FBX (.fbx)
- COLLADA (.dae)
- STL (.stl)
- And more formats supported by Assimp

### Texture Support
- Diffuse maps (albedo)
- Specular maps
- Normal maps
- Ambient Occlusion maps
- Height maps
- Supported formats: JPG, PNG, BMP, TGA

## Shaders
- **Vertex Shader (`vertex.glsl`)**:
  - Transforms vertex positions and passes normals to the fragment shader
  - Handles skeletal animations and vertex skinning
- **Fragment Shader (`fragment.glsl`)**:
  - Calculates PBR lighting (ambient, diffuse, and specular)
  - Applies material textures and normal mapping
  - Handles shadow mapping

## Lighting
- **Directional Light**:
  - The light direction is defined as `(-0.2f, -1.0f, -0.3f)`
  - The light color is white (`1.0f, 1.0f, 1.0f`)
- **Shadows**:
  - Dynamic shadow mapping for all objects in the scene
  - Soft shadows with PCF filtering

## Future Improvements
- **PBR Materials**: Implement physically based rendering materials
- **Post-Processing**: Add post-processing effects (bloom, ambient occlusion, etc.)
- **Multiple Lights**: Add support for multiple light sources (point lights, spotlights)
- **Animation System**: Implement skeletal animation support

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---
