#pragma once

// MistEngine Version Information
#define MIST_ENGINE_VERSION_MAJOR 0
#define MIST_ENGINE_VERSION_MINOR 3
#define MIST_ENGINE_VERSION_PATCH 0
#define MIST_ENGINE_VERSION_BUILD "pre-alha"

#define MIST_ENGINE_VERSION_STRING "0.3.0-prealpha"
#define MIST_ENGINE_NAME "MistEngine"
#define MIST_ENGINE_DESCRIPTION "A Modern C++ Game Engine with AI Integration and FPS Game Mode"

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

// Features available in this build
#define MIST_ENGINE_HAS_AI_INTEGRATION 1
#define MIST_ENGINE_HAS_PHYSICS 1
#define MIST_ENGINE_HAS_OPENGL 1
#define MIST_ENGINE_HAS_IMGUI 1
#define MIST_ENGINE_HAS_FPS_GAME 1

// Build date/time (set by build system)
#ifndef MIST_ENGINE_BUILD_DATE
    #define MIST_ENGINE_BUILD_DATE __DATE__
#endif

#ifndef MIST_ENGINE_BUILD_TIME
    #define MIST_ENGINE_BUILD_TIME __TIME__
#endif