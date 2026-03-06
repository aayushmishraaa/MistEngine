#pragma once
#ifndef MIST_PROFILER_H
#define MIST_PROFILER_H

#include <glad/glad.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

struct ProfileSection {
    std::string name;
    float cpuTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
};

class Profiler {
public:
    static constexpr int FPS_HISTORY_SIZE = 120;
    static constexpr int MAX_GPU_QUERIES = 32;

    void Init();
    void Shutdown();

    // CPU timing
    void BeginCPUSection(const std::string& name);
    void EndCPUSection(const std::string& name);

    // GPU timing (uses GL_TIME_ELAPSED queries)
    void BeginGPUSection(const std::string& name);
    void EndGPUSection(const std::string& name);

    // Frame management
    void BeginFrame();
    void EndFrame();

    // Results
    const std::vector<ProfileSection>& GetSections() const { return m_Sections; }

    // FPS
    float GetFPS() const { return m_FPS; }
    float GetFrameTimeMs() const { return m_FrameTimeMs; }
    const float* GetFPSHistory() const { return m_FPSHistory; }
    int GetFPSHistorySize() const { return FPS_HISTORY_SIZE; }
    int GetFPSHistoryOffset() const { return m_FPSHistoryIdx; }

    // Draw call counter
    void IncrementDrawCalls(int count = 1) { m_DrawCalls += count; }
    void ResetDrawCalls() { m_DrawCalls = 0; }
    int GetDrawCalls() const { return m_DrawCalls; }

    // Triangle counter
    void AddTriangles(int count) { m_Triangles += count; }
    void ResetTriangles() { m_Triangles = 0; }
    int GetTriangles() const { return m_Triangles; }

    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }

private:
    bool m_Enabled = true;

    // CPU timing
    struct CPUTimer {
        std::chrono::high_resolution_clock::time_point start;
    };
    std::unordered_map<std::string, CPUTimer> m_CPUTimers;

    // GPU timing
    struct GPUQuery {
        GLuint queryID = 0;
        std::string name;
        bool active = false;
    };
    std::vector<GPUQuery> m_GPUQueries;
    int m_NextQueryIndex = 0;

    // Results
    std::vector<ProfileSection> m_Sections;
    std::unordered_map<std::string, int> m_SectionIndex;

    // FPS tracking
    float m_FPS = 0.0f;
    float m_FrameTimeMs = 0.0f;
    float m_FPSHistory[FPS_HISTORY_SIZE] = {};
    int m_FPSHistoryIdx = 0;
    std::chrono::high_resolution_clock::time_point m_FrameStart;

    // Counters
    int m_DrawCalls = 0;
    int m_Triangles = 0;

    ProfileSection& getOrCreateSection(const std::string& name);
};

#endif
