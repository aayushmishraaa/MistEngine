
#ifndef SHAPE_GENERATOR_H
#define SHAPE_GENERATOR_H

#include <vector>
#include "Mesh.h" // Include Vertex struct

// Function to generate cube mesh data
void generateCubeMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

// Function to generate plane mesh data
void generatePlaneMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

#endif // SHAPE_GENERATOR_H
