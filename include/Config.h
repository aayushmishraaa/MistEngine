#pragma once

// Build configuration defines
#define IMGUI_ENABLED 1  // Enable ImGui with proper vcpkg integration

// Engine version
#define MISTENGINE_VERSION_MAJOR 1
#define MISTENGINE_VERSION_MINOR 0
#define MISTENGINE_VERSION_PATCH 0

// Debug configuration
#ifdef _DEBUG
#define MISTENGINE_DEBUG 1
#define MISTENGINE_ENABLE_ASSERTS 1
#else
#define MISTENGINE_DEBUG 0
#define MISTENGINE_ENABLE_ASSERTS 0
#endif

// Platform defines
#ifdef _WIN32
#define MISTENGINE_PLATFORM_WINDOWS 1
#elif __linux__
#define MISTENGINE_PLATFORM_LINUX 1
#elif __APPLE__
#define MISTENGINE_PLATFORM_APPLE 1
#endif

// Renderer configuration
#define MISTENGINE_OPENGL 1
#define MISTENGINE_VULKAN 0
#define MISTENGINE_DIRECTX 0