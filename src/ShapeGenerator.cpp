#include "ShapeGenerator.h"
#include <glm/glm.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void computeTangentBitangent(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    // Initialize tangent/bitangent to zero
    for (auto& v : vertices) {
        v.Tangent = glm::vec3(0.0f);
        v.Bitangent = glm::vec3(0.0f);
    }

    // Accumulate per-triangle tangent/bitangent
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;
        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

        float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        float f = (std::abs(denom) > 1e-6f) ? (1.0f / denom) : 0.0f;

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;
        v0.Bitangent += bitangent;
        v1.Bitangent += bitangent;
        v2.Bitangent += bitangent;
    }

    // Normalize
    for (auto& v : vertices) {
        if (glm::length(v.Tangent) > 1e-6f)
            v.Tangent = glm::normalize(v.Tangent);
        if (glm::length(v.Bitangent) > 1e-6f)
            v.Bitangent = glm::normalize(v.Bitangent);
    }
}

void generateCubeMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    vertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {}, {}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {}, {}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {}, {}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {}, {}},
        // Back face
        {{ 0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {}, {}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {}, {}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {}, {}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {}, {}},
        // Left face
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {}, {}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {}, {}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {}, {}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {}, {}},
        // Right face
        {{ 0.5f, -0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {}, {}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {}, {}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {}, {}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {}, {}},
        // Top face
        {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {}, {}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {}, {}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {}, {}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {}, {}},
        // Bottom face
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {}, {}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {}, {}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {}, {}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {}, {}}
    };

    indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Left
        12, 13, 14, 14, 15, 12, // Right
        16, 17, 18, 18, 19, 16, // Top
        20, 21, 22, 22, 23, 20  // Bottom
    };

    computeTangentBitangent(vertices, indices);
}

void generatePlaneMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    vertices = {
        {{ 10.0f, 0.0f,  10.0f}, {0.0f, 1.0f, 0.0f}, {10.0f, 10.0f}, {}, {}},
        {{-10.0f, 0.0f,  10.0f}, {0.0f, 1.0f, 0.0f}, { 0.0f, 10.0f}, {}, {}},
        {{-10.0f, 0.0f, -10.0f}, {0.0f, 1.0f, 0.0f}, { 0.0f,  0.0f}, {}, {}},
        {{ 10.0f, 0.0f, -10.0f}, {0.0f, 1.0f, 0.0f}, {10.0f,  0.0f}, {}, {}}
    };

    indices = {
        0, 1, 2,
        0, 2, 3
    };

    computeTangentBitangent(vertices, indices);
}

void generateSphereMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, float radius, int sectorCount, int stackCount) {
    vertices.clear();
    indices.clear();

    float lengthInv = 1.0f / radius;
    float sectorStep = 2.0f * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;

    for (int i = 0; i <= stackCount; ++i) {
        float stackAngle = M_PI / 2.0f - i * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * sectorStep;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            float nx = x * lengthInv;
            float ny = y * lengthInv;
            float nz = z * lengthInv;

            float s = (float)j / sectorCount;
            float t = (float)i / stackCount;

            vertices.push_back({{x, y, z}, {nx, ny, nz}, {s, t}, {}, {}});
        }
    }

    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    computeTangentBitangent(vertices, indices);
}
