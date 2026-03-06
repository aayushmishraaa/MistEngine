#include "Debug/Profiler.h"
#include "Core/Logger.h"

void Profiler::Init() {
    m_GPUQueries.resize(MAX_GPU_QUERIES);
    for (auto& q : m_GPUQueries) {
        glCreateQueries(GL_TIME_ELAPSED, 1, &q.queryID);
    }
    LOG_INFO("Profiler initialized (", MAX_GPU_QUERIES, " GPU query slots)");
}

void Profiler::Shutdown() {
    for (auto& q : m_GPUQueries) {
        if (q.queryID) glDeleteQueries(1, &q.queryID);
    }
    m_GPUQueries.clear();
}

void Profiler::BeginCPUSection(const std::string& name) {
    if (!m_Enabled) return;
    m_CPUTimers[name].start = std::chrono::high_resolution_clock::now();
}

void Profiler::EndCPUSection(const std::string& name) {
    if (!m_Enabled) return;
    auto it = m_CPUTimers.find(name);
    if (it == m_CPUTimers.end()) return;

    auto end = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(end - it->second.start).count();

    auto& section = getOrCreateSection(name);
    section.cpuTimeMs = ms;
}

void Profiler::BeginGPUSection(const std::string& name) {
    if (!m_Enabled) return;
    if (m_NextQueryIndex >= MAX_GPU_QUERIES) return;

    auto& q = m_GPUQueries[m_NextQueryIndex];
    q.name = name;
    q.active = true;
    glBeginQuery(GL_TIME_ELAPSED, q.queryID);
}

void Profiler::EndGPUSection(const std::string& name) {
    if (!m_Enabled) return;
    if (m_NextQueryIndex >= MAX_GPU_QUERIES) return;

    glEndQuery(GL_TIME_ELAPSED);
    m_NextQueryIndex++;
}

void Profiler::BeginFrame() {
    if (!m_Enabled) return;

    m_FrameStart = std::chrono::high_resolution_clock::now();

    // Collect previous frame's GPU results
    for (int i = 0; i < m_NextQueryIndex; i++) {
        auto& q = m_GPUQueries[i];
        if (!q.active) continue;

        GLuint64 gpuTime = 0;
        glGetQueryObjectui64v(q.queryID, GL_QUERY_RESULT, &gpuTime);

        auto& section = getOrCreateSection(q.name);
        section.gpuTimeMs = static_cast<float>(gpuTime) / 1e6f; // ns to ms
        q.active = false;
    }

    m_NextQueryIndex = 0;
    ResetDrawCalls();
    ResetTriangles();
}

void Profiler::EndFrame() {
    if (!m_Enabled) return;

    auto now = std::chrono::high_resolution_clock::now();
    m_FrameTimeMs = std::chrono::duration<float, std::milli>(now - m_FrameStart).count();
    m_FPS = (m_FrameTimeMs > 0.0f) ? 1000.0f / m_FrameTimeMs : 0.0f;

    m_FPSHistory[m_FPSHistoryIdx] = m_FPS;
    m_FPSHistoryIdx = (m_FPSHistoryIdx + 1) % FPS_HISTORY_SIZE;
}

ProfileSection& Profiler::getOrCreateSection(const std::string& name) {
    auto it = m_SectionIndex.find(name);
    if (it != m_SectionIndex.end()) return m_Sections[it->second];

    int idx = static_cast<int>(m_Sections.size());
    m_SectionIndex[name] = idx;
    m_Sections.push_back({name});
    return m_Sections.back();
}
