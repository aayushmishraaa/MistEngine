#pragma once

// MistEngine Version Information
#define MIST_ENGINE_VERSION_MAJOR 0
#define MIST_ENGINE_VERSION_MINOR 5
#define MIST_ENGINE_VERSION_PATCH 0
#define MIST_ENGINE_VERSION_BUILD "pre-alpha"

#define MIST_ENGINE_VERSION_STRING "0.5.0-prealpha"
#define MIST_ENGINE_NAME "MistEngine"
#define MIST_ENGINE_DESCRIPTION "A Modern C++17 OpenGL 4.6 Engine — editor + renderer sandbox with PBR, HDR, TAA, SSGI and PCSS"

// Build information
#ifdef _DEBUG
    #define MIST_ENGINE_BUILD_TYPE "Debug"
#else
    #define MIST_ENGINE_BUILD_TYPE "Release"
#endif

#ifdef _WIN64
    #define MIST_ENGINE_PLATFORM "Windows x64"
#elif defined(_WIN32)
    #define MIST_ENGINE_PLATFORM "Windows x86"
#elif defined(__linux__)
    #ifdef __x86_64__
        #define MIST_ENGINE_PLATFORM "Linux x86_64"
    #elif defined(__aarch64__)
        #define MIST_ENGINE_PLATFORM "Linux aarch64"
    #else
        #define MIST_ENGINE_PLATFORM "Linux"
    #endif
#else
    #define MIST_ENGINE_PLATFORM "Unknown"
#endif

// Compiler information
#ifdef _MSC_VER
    #define MIST_ENGINE_COMPILER "MSVC"
#elif defined(__GNUC__)
    #define MIST_ENGINE_COMPILER "GCC"
#elif defined(__clang__)
    #define MIST_ENGINE_COMPILER "Clang"
#else
    #define MIST_ENGINE_COMPILER "Unknown"
#endif

// Features available in this build.
//
// MIST_ENGINE_HAS_AI_INTEGRATION is gone, not set to 0: the Gemini/OpenAI
// subsystem was deleted in 0.5.0, but this flag stayed at 1, so main()'s
// startup banner kept printing "AI" as an available feature of a build that
// has no such code in it. MIST_ENGINE_HAS_FPS_GAME below is the same story and
// is removed for the same reason.
#define MIST_ENGINE_HAS_PHYSICS 1
#define MIST_ENGINE_HAS_OPENGL 1
#define MIST_ENGINE_HAS_IMGUI 1
#define MIST_ENGINE_HAS_PBR 1
#define MIST_ENGINE_HAS_HDR 1
#define MIST_ENGINE_HAS_POST_PROCESSING 1
#define MIST_ENGINE_HAS_CSM_SHADOWS 1
#define MIST_ENGINE_HAS_CLUSTERED_LIGHTING 1
#define MIST_ENGINE_HAS_IBL 1
#define MIST_ENGINE_HAS_GPU_PARTICLES 1
#define MIST_ENGINE_HAS_SKELETAL_ANIMATION 1
#define MIST_ENGINE_HAS_TAA 1
#define MIST_ENGINE_HAS_SSGI 1
#define MIST_ENGINE_HAS_PCSS 1
#define MIST_ENGINE_HAS_SCENE_SERIALIZATION 1

// Build date/time (set by build system)
#ifndef MIST_ENGINE_BUILD_DATE
    #define MIST_ENGINE_BUILD_DATE __DATE__
#endif

#ifndef MIST_ENGINE_BUILD_TIME
    #define MIST_ENGINE_BUILD_TIME __TIME__
#endif