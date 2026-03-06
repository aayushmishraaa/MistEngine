#pragma once
#ifndef MIST_IBL_H
#define MIST_IBL_H

#include <glad/glad.h>
#include <string>
#include "Shader.h"

class IBL {
public:
    IBL() = default;
    ~IBL();

    bool ProcessEnvironmentMap(const std::string& hdrPath);
    void Bind(Shader& shader, int irradianceUnit = 10, int prefilterUnit = 11, int brdfLUTUnit = 12);

    GLuint GetEnvCubemap() const { return m_EnvCubemap; }
    GLuint GetIrradianceMap() const { return m_IrradianceMap; }
    GLuint GetPrefilterMap() const { return m_PrefilterMap; }
    GLuint GetBRDFLUT() const { return m_BRDFLUT; }

    bool IsLoaded() const { return m_Loaded; }

private:
    GLuint m_EnvCubemap = 0;
    GLuint m_IrradianceMap = 0;
    GLuint m_PrefilterMap = 0;
    GLuint m_BRDFLUT = 0;
    GLuint m_CaptureFBO = 0;
    GLuint m_CaptureRBO = 0;
    bool m_Loaded = false;

    void generateBRDFLUT();
    void cleanup();
};

#endif
