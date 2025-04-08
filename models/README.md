# 3D Models Directory

This directory is used to store 3D model files that can be loaded into MistEngine.

## Supported Formats

MistEngine uses Assimp (Open Asset Import Library) for model loading, which supports various 3D file formats including:

- Wavefront OBJ (.obj)
- FBX (.fbx)
- COLLADA (.dae)
- 3DS (.3ds)
- And many more...

## Model Requirements

- Models should include:
  - Vertex positions
  - Normal data
  - UV coordinates (for texturing)
- Texture files should be placed in the same directory as the model
- Recommended model polygon count: < 100k triangles for optimal performance

## Usage

1. Place your 3D model files in this directory
2. Place associated texture files in the same directory
3. Update the model path in MistEngine.cpp to load your model:
   ```cpp
   model = new Model("models/your_model.obj");
   ```

## Example Models

You can find free 3D models for testing from various sources:
- [Sketchfab](https://sketchfab.com/)
- [TurboSquid](https://www.turbosquid.com/)
- [Free3D](https://free3d.com/)

Make sure to check the licensing terms before using any 3D models in your project.