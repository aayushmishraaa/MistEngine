#ifndef SHAPE_GENERATOR_H
#define SHAPE_GENERATOR_H

#include <vector>
#include "Mesh.h" // Include Vertex struct

// Function to generate cube mesh data
void generateCubeMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

// Function to generate plane mesh data
void generatePlaneMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

// Function to generate sphere mesh data
void generateSphereMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, float radius = 1.0f, int sectorCount = 36, int stackCount = 18);

#endif // SHAPE_GENERATOR_H
