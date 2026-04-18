#include "GameExporter.h"
#include "Version.h"
#include "Core/PathGuard.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <vector>
#include <cstdint>
#include <filesystem>
#ifdef _WIN32
#include <direct.h>
#endif
#include <sys/stat.h>

namespace {
// Exports go under ./exports. Keeping the sandbox narrow means a hostile
// ExportSettings.outputDirectory of "../../" can't be used to spray files
// outside the project.
std::filesystem::path ExportSandboxRoot() {
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (ec) {
        return std::filesystem::path{"exports"};
    }
    return cwd / "exports";
}
} // namespace

GameExporter::GameExporter()
    : m_isExporting(false)
    , m_exportProgress(0.0f)
    , m_exportStatus("Ready")
{
}

GameExporter::~GameExporter() {
}

bool GameExporter::ExportGame(const ExportSettings& settings) {
    if (m_isExporting) {
        std::cerr << "Export already in progress" << std::endl;
        return false;
    }
    
    m_isExporting = true;
    UpdateExportProgress(0.0f, "Starting export...");
    
    // Validate settings
    if (!ValidateExportSettings(settings)) {
        UpdateExportProgress(0.0f, "Export failed: Invalid settings");
        m_isExporting = false;
        return false;
    }
    
    // Create output directory. Resolve under the export sandbox so a malicious
    // outputDirectory (e.g. "../../etc") can't escape the project tree.
    const auto sandbox = ExportSandboxRoot();
    std::error_code sandbox_ec;
    std::filesystem::create_directories(sandbox, sandbox_ec);

    std::filesystem::path requested = std::filesystem::path(settings.outputDirectory) /
                                      std::filesystem::path(settings.gameName);
    std::filesystem::path resolvedOut;
    if (!Mist::PathGuard::is_under(sandbox, requested, &resolvedOut)) {
        UpdateExportProgress(0.0f, "Export failed: output path escapes sandbox");
        m_isExporting = false;
        return false;
    }
    std::string outputPath = resolvedOut.generic_string();
    if (!CreateDirectory(outputPath)) {
        UpdateExportProgress(0.0f, "Export failed: Could not create output directory");
        m_isExporting = false;
        return false;
    }
    
    UpdateExportProgress(10.0f, "Created output directory");
    
    // Copy engine files
    if (!CopyEngineFiles(outputPath)) {
        UpdateExportProgress(0.0f, "Export failed: Could not copy engine files");
        m_isExporting = false;
        return false;
    }
    
    UpdateExportProgress(30.0f, "Copied engine files");
    
    // Copy and package assets
    if (settings.includeAssets) {
        if (!CopyGameAssets(outputPath)) {
            UpdateExportProgress(0.0f, "Export failed: Could not copy assets");
            m_isExporting = false;
            return false;
        }
        
        if (settings.compressAssets) {
            if (!CompressAssets(outputPath + "/assets", outputPath + "/game_assets.pak")) {
                std::cout << "Warning: Asset compression failed, using uncompressed assets" << std::endl;
            } else {
                UpdateExportProgress(50.0f, "Compressed assets");
            }
        }
    }
    
    UpdateExportProgress(60.0f, "Processed assets");
    
    // Generate configuration files
    if (!GenerateGameConfig(settings, outputPath)) {
        UpdateExportProgress(0.0f, "Export failed: Could not generate config");
        m_isExporting = false;
        return false;
    }
    
    UpdateExportProgress(70.0f, "Generated configuration");
    
    // Generate level data
    if (!GenerateLevelData(settings, outputPath)) {
        UpdateExportProgress(0.0f, "Export failed: Could not generate level data");
        m_isExporting = false;
        return false;
    }
    
    UpdateExportProgress(80.0f, "Generated level data");
    
    // Create launcher
    if (!CreateLauncherExecutable(outputPath, settings)) {
        UpdateExportProgress(0.0f, "Export failed: Could not create launcher");
        m_isExporting = false;
        return false;
    }
    
    UpdateExportProgress(90.0f, "Created launcher");
    
    // Create readme
    if (!CreateReadme(outputPath, settings)) {
        std::cout << "Warning: Could not create readme file" << std::endl;
    }
    
    UpdateExportProgress(100.0f, "Export complete!");
    m_isExporting = false;
    
    std::cout << "Game exported successfully to: " << outputPath << std::endl;
    return true;
}

bool GameExporter::ExportStandalone(const std::string& outputPath) {
    ExportSettings defaultSettings;
    defaultSettings.outputDirectory = outputPath;
    return ExportGame(defaultSettings);
}

bool GameExporter::ExportWithInstaller(const std::string& outputPath) {
    ExportSettings settings;
    settings.outputDirectory = outputPath;
    settings.createInstaller = true;
    return ExportGame(settings);
}

bool GameExporter::ValidateExportSettings(const ExportSettings& settings) {
    if (settings.gameName.empty()) {
        std::cerr << "Game name cannot be empty" << std::endl;
        return false;
    }
    
    if (settings.numberOfLevels <= 0) {
        std::cerr << "Number of levels must be greater than 0" << std::endl;
        return false;
    }
    
    if (settings.enemiesPerLevel <= 0) {
        std::cerr << "Enemies per level must be greater than 0" << std::endl;
        return false;
    }
    
    return true;
}

std::vector<std::string> GameExporter::GetRequiredFiles() const {
#ifdef _WIN32
    return {
        "MistEngine.exe",
        "glfw3.dll",
        "opengl32.dll",
        "shaders/",
        "textures/",
        "models/",
    };
#else
    return {
        "MistEngine",
        "shaders/",
        "textures/",
        "models/",
    };
#endif
}

bool GameExporter::CopyEngineFiles(const std::string& outputPath) {
    // This is a simplified implementation
    // In a real exporter, you would copy the actual engine executable and dependencies
    
    std::cout << "Copying engine files to: " << outputPath << std::endl;
    
    // Create subdirectories
    CreateDirectory(outputPath + "/shaders");
    CreateDirectory(outputPath + "/textures");
    CreateDirectory(outputPath + "/assets");
    
    // Copy shader files (shaders are in the build root directory)
    const char* shaderFiles[] = {
        "pbr_vertex.glsl", "pbr_fragment.glsl",
        "vertex.glsl", "fragment.glsl",
        "depth_vertex.glsl", "depth_fragment.glsl",
        "skybox_vertex.glsl", "skybox_fragment.glsl",
        nullptr
    };
    for (int i = 0; shaderFiles[i]; ++i) {
        if (!CopyFile(shaderFiles[i], outputPath + "/shaders/" + shaderFiles[i])) {
            std::cout << "Warning: Could not copy shader " << shaderFiles[i] << std::endl;
        }
    }

    // Copy the actual engine executable
    bool copiedExe = CopyFile("MistEngine", outputPath + "/MistFPS");
    if (copiedExe) {
#ifndef _WIN32
        chmod((outputPath + "/MistFPS").c_str(), 0755);
#endif
        std::cout << "Copied engine executable as MistFPS" << std::endl;
    } else {
        std::cout << "Warning: Could not copy engine executable, creating placeholder" << std::endl;
        WriteTextFile(outputPath + "/MistFPS.txt", "# MistFPS Placeholder\n");
    }

    return true;
}

// --- Procedural asset generation helpers ---

static void WriteBMP(const std::string& path, int w, int h, const std::vector<uint8_t>& pixels) {
    int rowSize = (w * 3 + 3) & ~3; // rows padded to 4-byte boundary
    int dataSize = rowSize * h;
    int fileSize = 54 + dataSize;

    uint8_t header[54] = {};
    header[0] = 'B'; header[1] = 'M';
    std::memcpy(&header[2], &fileSize, 4);
    int offset = 54; std::memcpy(&header[10], &offset, 4);
    int infoSize = 40; std::memcpy(&header[14], &infoSize, 4);
    std::memcpy(&header[18], &w, 4);
    std::memcpy(&header[22], &h, 4);
    int16_t planes = 1; std::memcpy(&header[26], &planes, 2);
    int16_t bpp = 24; std::memcpy(&header[28], &bpp, 2);
    std::memcpy(&header[34], &dataSize, 4);

    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<char*>(header), 54);
    std::vector<uint8_t> row(rowSize, 0);
    for (int y = h - 1; y >= 0; --y) { // BMP is bottom-up
        for (int x = 0; x < w; ++x) {
            int si = (y * w + x) * 3;
            row[x * 3 + 0] = pixels[si + 2]; // B
            row[x * 3 + 1] = pixels[si + 1]; // G
            row[x * 3 + 2] = pixels[si + 0]; // R
        }
        f.write(reinterpret_cast<char*>(row.data()), rowSize);
    }
}

static void GenerateTextureBMP(const std::string& path, int w, int h,
                                uint8_t r, uint8_t g, uint8_t b, bool checkerboard) {
    std::vector<uint8_t> pixels(w * h * 3);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = (y * w + x) * 3;
            if (checkerboard && ((x / 8 + y / 8) % 2 == 0)) {
                pixels[i]     = std::min(255, r + 40);
                pixels[i + 1] = std::min(255, g + 40);
                pixels[i + 2] = std::min(255, b + 40);
            } else {
                pixels[i] = r; pixels[i + 1] = g; pixels[i + 2] = b;
            }
        }
    }
    WriteBMP(path, w, h, pixels);
}

static void WriteWAV(const std::string& path, const std::vector<int16_t>& samples, int sampleRate) {
    int dataSize = (int)samples.size() * 2;
    int fileSize = 36 + dataSize;
    int16_t channels = 1, bitsPerSample = 16;
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    int16_t blockAlign = channels * bitsPerSample / 8;

    std::ofstream f(path, std::ios::binary);
    f.write("RIFF", 4);
    f.write(reinterpret_cast<char*>(&fileSize), 4);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    int fmtSize = 16; f.write(reinterpret_cast<char*>(&fmtSize), 4);
    int16_t audioFmt = 1; f.write(reinterpret_cast<char*>(&audioFmt), 2);
    f.write(reinterpret_cast<char*>(&channels), 2);
    f.write(reinterpret_cast<char*>(&sampleRate), 4);
    f.write(reinterpret_cast<char*>(&byteRate), 4);
    f.write(reinterpret_cast<char*>(&blockAlign), 2);
    f.write(reinterpret_cast<char*>(&bitsPerSample), 2);
    f.write("data", 4);
    f.write(reinterpret_cast<char*>(&dataSize), 4);
    f.write(reinterpret_cast<const char*>(samples.data()), dataSize);
}

static void GenerateGunshot(const std::string& path) {
    const int rate = 22050;
    const int len = rate / 5; // 0.2 seconds
    std::vector<int16_t> samples(len);
    for (int i = 0; i < len; ++i) {
        float t = (float)i / rate;
        float env = std::exp(-t * 30.0f); // fast decay
        float noise = ((float)(rand() % 32768) / 16384.0f - 1.0f);
        float low = std::sin(2.0f * 3.14159f * 80.0f * t); // bass thump
        samples[i] = (int16_t)((noise * 0.7f + low * 0.3f) * env * 28000);
    }
    WriteWAV(path, samples, rate);
}

static void GenerateReload(const std::string& path) {
    const int rate = 22050;
    const int len = rate / 3; // 0.33 seconds
    std::vector<int16_t> samples(len);
    for (int i = 0; i < len; ++i) {
        float t = (float)i / rate;
        float click1 = (t < 0.02f) ? std::sin(2.0f * 3.14159f * 2000.0f * t) * (1.0f - t / 0.02f) : 0.0f;
        float click2 = (t > 0.15f && t < 0.18f) ? std::sin(2.0f * 3.14159f * 1500.0f * t) * (1.0f - (t - 0.15f) / 0.03f) : 0.0f;
        float slide = (t > 0.05f && t < 0.13f) ? ((float)(rand() % 32768) / 16384.0f - 1.0f) * 0.2f * (1.0f - (t - 0.05f) / 0.08f) : 0.0f;
        samples[i] = (int16_t)((click1 + click2 + slide) * 20000);
    }
    WriteWAV(path, samples, rate);
}

static void GenerateEnemyDeath(const std::string& path) {
    const int rate = 22050;
    const int len = rate / 2; // 0.5 seconds
    std::vector<int16_t> samples(len);
    for (int i = 0; i < len; ++i) {
        float t = (float)i / rate;
        float env = std::exp(-t * 6.0f);
        float freq = 400.0f - 300.0f * t; // descending tone
        float tone = std::sin(2.0f * 3.14159f * freq * t);
        float noise = ((float)(rand() % 32768) / 16384.0f - 1.0f) * 0.3f;
        samples[i] = (int16_t)((tone * 0.6f + noise * 0.4f) * env * 22000);
    }
    WriteWAV(path, samples, rate);
}

static void GenerateOBJ(const std::string& path, const std::string& name, float sx, float sy, float sz) {
    std::ostringstream obj;
    obj << "# " << name << " - Generated by MistEngine Exporter\n";
    obj << "o " << name << "\n";
    // Simple box mesh
    float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;
    obj << "v " << -hx << " " << -hy << " " <<  hz << "\n";
    obj << "v " <<  hx << " " << -hy << " " <<  hz << "\n";
    obj << "v " <<  hx << " " <<  hy << " " <<  hz << "\n";
    obj << "v " << -hx << " " <<  hy << " " <<  hz << "\n";
    obj << "v " << -hx << " " << -hy << " " << -hz << "\n";
    obj << "v " <<  hx << " " << -hy << " " << -hz << "\n";
    obj << "v " <<  hx << " " <<  hy << " " << -hz << "\n";
    obj << "v " << -hx << " " <<  hy << " " << -hz << "\n";
    obj << "vn 0 0 1\nvn 0 0 -1\nvn -1 0 0\nvn 1 0 0\nvn 0 1 0\nvn 0 -1 0\n";
    obj << "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n";
    obj << "f 1/1/1 2/2/1 3/3/1 4/4/1\n";
    obj << "f 6/1/2 5/2/2 8/3/2 7/4/2\n";
    obj << "f 5/1/3 1/2/3 4/3/3 8/4/3\n";
    obj << "f 2/1/4 6/2/4 7/3/4 3/4/4\n";
    obj << "f 4/1/5 3/2/5 7/3/5 8/4/5\n";
    obj << "f 5/1/6 6/2/6 2/3/6 1/4/6\n";

    std::ofstream f(path);
    f << obj.str();
}

// --- End procedural asset helpers ---

bool GameExporter::CopyGameAssets(const std::string& outputPath) {
    std::cout << "Generating game assets to: " << outputPath << "/assets" << std::endl;

    CreateDirectory(outputPath + "/assets");
    CreateDirectory(outputPath + "/assets/models");
    CreateDirectory(outputPath + "/assets/textures");
    CreateDirectory(outputPath + "/assets/sounds");

    // Generate real OBJ model files
    GenerateOBJ(outputPath + "/assets/models/player.obj", "Player", 0.8f, 1.8f, 0.6f);
    GenerateOBJ(outputPath + "/assets/models/enemy.obj", "Enemy", 1.0f, 1.8f, 0.8f);
    GenerateOBJ(outputPath + "/assets/models/weapons.obj", "Weapon", 0.15f, 0.15f, 0.8f);

    // Generate real BMP texture files (64x64 checkerboard patterns)
    GenerateTextureBMP(outputPath + "/assets/textures/player.bmp", 64, 64, 180, 140, 100, true);
    GenerateTextureBMP(outputPath + "/assets/textures/enemy.bmp", 64, 64, 200, 50, 40, true);
    GenerateTextureBMP(outputPath + "/assets/textures/weapons.bmp", 64, 64, 120, 120, 130, true);
    GenerateTextureBMP(outputPath + "/assets/textures/floor.bmp", 64, 64, 100, 95, 85, true);
    GenerateTextureBMP(outputPath + "/assets/textures/wall.bmp", 64, 64, 140, 115, 100, true);

    // Generate real WAV sound files (procedurally synthesized)
    GenerateGunshot(outputPath + "/assets/sounds/gunshot.wav");
    GenerateReload(outputPath + "/assets/sounds/reload.wav");
    GenerateEnemyDeath(outputPath + "/assets/sounds/enemy_death.wav");

    std::cout << "Generated 3 OBJ models, 5 BMP textures, 3 WAV sounds" << std::endl;
    return true;
}

bool GameExporter::GenerateGameConfig(const ExportSettings& settings, const std::string& outputPath) {
    std::ostringstream config;
    config << "# MistFPS Game Configuration\n";
    config << "# Generated by MistEngine Exporter\n\n";
    
    config << "[Game]\n";
    config << "Name=" << settings.gameName << "\n";
    config << "Version=" << settings.version << "\n";
    config << "NumberOfLevels=" << settings.numberOfLevels << "\n";
    config << "EnemiesPerLevel=" << settings.enemiesPerLevel << "\n\n";
    
    config << "[Graphics]\n";
    config << "ScreenWidth=1200\n";
    config << "ScreenHeight=800\n";
    config << "Fullscreen=false\n";
    config << "VSync=true\n\n";
    
    config << "[Audio]\n";
    config << "MasterVolume=1.0\n";
    config << "SFXVolume=1.0\n";
    config << "MusicVolume=0.8\n\n";
    
    config << "[Controls]\n";
    config << "MouseSensitivity=0.1\n";
    config << "InvertY=false\n\n";
    
    config << "[Weapons]\n";
    for (size_t i = 0; i < settings.weaponTypes.size(); ++i) {
        config << "Weapon" << i << "=" << settings.weaponTypes[i] << "\n";
    }
    
    return WriteTextFile(outputPath + "/game_config.ini", config.str());
}

bool GameExporter::GenerateLevelData(const ExportSettings& settings, const std::string& outputPath) {
    CreateDirectory(outputPath + "/levels");
    
    for (int i = 1; i <= settings.numberOfLevels; ++i) {
        std::ostringstream levelData;
        levelData << "# Level " << i << " Data\n";
        levelData << "# Generated by MistEngine Exporter\n\n";
        
        levelData << "[LevelInfo]\n";
        levelData << "Name=Level " << i << "\n";
        levelData << "Description=Fight through room " << i << " and defeat all enemies\n";
        levelData << "Difficulty=" << (float)i / settings.numberOfLevels << "\n\n";
        
        levelData << "[PlayerStart]\n";
        levelData << "X=0.0\n";
        levelData << "Y=1.0\n";
        levelData << "Z=0.0\n\n";
        
        levelData << "[Enemies]\n";
        levelData << "Count=" << settings.enemiesPerLevel + (i - 1) * 2 << "\n";
        
        // Generate enemy spawn points
        for (int j = 0; j < settings.enemiesPerLevel + (i - 1) * 2; ++j) {
            levelData << "Enemy" << j << "_Type=";
            if (j % 3 == 0) levelData << "Grunt\n";
            else if (j % 3 == 1) levelData << "Soldier\n";
            else levelData << "Heavy\n";
            
            levelData << "Enemy" << j << "_X=" << (j % 5 - 2) * 8.0f << "\n";
            levelData << "Enemy" << j << "_Y=1.0\n";
            levelData << "Enemy" << j << "_Z=" << (j / 5) * 8.0f + 10.0f << "\n";
        }
        
        levelData << "\n[Objectives]\n";
        levelData << "Primary=Eliminate all enemies\n";
        levelData << "Secondary=Complete without taking damage\n";
        
        std::string filename = outputPath + "/levels/level" + std::to_string(i) + ".ini";
        if (!WriteTextFile(filename, levelData.str())) {
            return false;
        }
    }
    
    return true;
}

bool GameExporter::CreateLauncherExecutable(const std::string& outputPath, const ExportSettings& settings) {
#ifdef _WIN32
    std::ostringstream launcher;
    launcher << "@echo off\n";
    launcher << "echo Starting " << settings.gameName << " v" << settings.version << "\n";
    launcher << "echo Built with MistEngine\n";
    launcher << "echo.\n";
    launcher << "echo Controls:\n";
    launcher << "echo   WASD - Move\n";
    launcher << "echo   Mouse - Look\n";
    launcher << "echo   Left Click - Shoot\n";
    launcher << "echo   R - Reload\n";
    launcher << "echo   1/2 - Switch Weapons\n";
    launcher << "echo   ESC - Pause\n";
    launcher << "echo.\n";
    launcher << "echo Press any key to start the game...\n";
    launcher << "pause >nul\n";
    launcher << "echo Starting game...\n";
    launcher << "MistFPS.exe\n";
    launcher << "pause\n";

    return WriteTextFile(outputPath + "/Launch_" + settings.gameName + ".bat", launcher.str());
#else
    std::ostringstream launcher;
    launcher << "#!/bin/bash\n";
    launcher << "echo \"Starting " << settings.gameName << " v" << settings.version << "\"\n";
    launcher << "echo \"Built with MistEngine\"\n";
    launcher << "echo \"\"\n";
    launcher << "echo \"Controls:\"\n";
    launcher << "echo \"  WASD - Move\"\n";
    launcher << "echo \"  Mouse - Look\"\n";
    launcher << "echo \"  Left Click - Shoot\"\n";
    launcher << "echo \"  R - Reload\"\n";
    launcher << "echo \"  1/2 - Switch Weapons\"\n";
    launcher << "echo \"  ESC - Pause\"\n";
    launcher << "echo \"\"\n";
    launcher << "DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n";
    launcher << "cd \"$DIR\"\n";
    launcher << "exec ./MistFPS \"$@\"\n";

    std::string scriptPath = outputPath + "/launch_" + settings.gameName + ".sh";
    if (!WriteTextFile(scriptPath, launcher.str())) {
        return false;
    }
    // Make the script executable
    chmod(scriptPath.c_str(), 0755);
    return true;
#endif
}

bool GameExporter::CreateReadme(const std::string& outputPath, const ExportSettings& settings) {
    std::ostringstream readme;
    readme << "# " << settings.gameName << " v" << settings.version << "\n\n";
    readme << "A first-person shooter game built with " << MIST_ENGINE_NAME << " " << MIST_ENGINE_VERSION_STRING << ".\n\n";
    readme << "## System Requirements\n";
#ifdef _WIN32
    readme << "- Windows 10 or later\n";
#else
    readme << "- Linux (Ubuntu 22.04+ or equivalent)\n";
    readme << "- libglfw3, libassimp, libbullet, libcurl installed\n";
#endif
    readme << "- OpenGL 3.3 compatible graphics card\n";
    readme << "- 2GB RAM minimum\n";
    readme << "- 500MB disk space\n\n";
    readme << "## How to Play\n";
#ifdef _WIN32
    readme << "1. Run Launch_" << settings.gameName << ".bat to start the game\n";
#else
    readme << "1. Run ./launch_" << settings.gameName << ".sh to start the game\n";
#endif
    readme << "2. Use WASD keys to move your character\n";
    readme << "3. Use mouse to look around\n";
    readme << "4. Left-click to shoot enemies\n";
    readme << "5. Press R to reload your weapon\n";
    readme << "6. Use number keys 1-4 to switch weapons\n";
    readme << "7. Defeat all enemies in each level to progress\n\n";
    readme << "## Game Features\n";
    readme << "- " << settings.numberOfLevels << " challenging levels\n";
    readme << "- Multiple weapon types: ";
    for (size_t i = 0; i < settings.weaponTypes.size(); ++i) {
        readme << settings.weaponTypes[i];
        if (i < settings.weaponTypes.size() - 1) readme << ", ";
    }
    readme << "\n";
    readme << "- Intelligent enemy AI with different behaviors\n";
    readme << "- Health and ammunition management\n";
    readme << "- Score system based on performance\n\n";
    readme << "## Troubleshooting\n";
    readme << "- If the game doesn't start, ensure you have the latest graphics drivers\n";
    readme << "- Check that all files are present in the game directory\n";
    readme << "- Run the game as administrator if needed\n\n";
    readme << "## Credits\n";
    readme << "Built with " << MIST_ENGINE_NAME << " " << MIST_ENGINE_VERSION_STRING << " - A modern C++14 game engine\n";
    readme << "Platform: " << MIST_ENGINE_PLATFORM << "\n";
    readme << "Compiler: " << MIST_ENGINE_COMPILER << "\n";
    readme << "Features: ";
    #if MIST_ENGINE_HAS_AI_INTEGRATION
    readme << "AI-Integration ";
    #endif
    #if MIST_ENGINE_HAS_PHYSICS
    readme << "Physics ";
    #endif
    #if MIST_ENGINE_HAS_OPENGL
    readme << "OpenGL ";
    #endif
    #if MIST_ENGINE_HAS_FPS_GAME
    readme << "FPS-Game ";
    #endif
    readme << "\n";
    readme << "Exported on: " << MIST_ENGINE_BUILD_DATE << " at " << MIST_ENGINE_BUILD_TIME << "\n";
    readme << "Engine Build: " << MIST_ENGINE_BUILD_TYPE << "\n";
    
    return WriteTextFile(outputPath + "/README.txt", readme.str());
}

bool GameExporter::CompressAssets(const std::string& inputPath, const std::string& outputPath) {
    // This is a placeholder for asset compression
    // In a real implementation, you would use a compression library like zlib
    std::cout << "Asset compression would be implemented here" << std::endl;
    std::cout << "Input: " << inputPath << std::endl;
    std::cout << "Output: " << outputPath << std::endl;
    return true;
}

bool GameExporter::CreateDirectory(const std::string& path) {
    try {
        // Recursively create parent directories first
        for (size_t pos = 1; pos < path.size(); ++pos) {
            if (path[pos] == '/') {
                std::string parent = path.substr(0, pos);
#ifdef _WIN32
                _mkdir(parent.c_str());
#else
                mkdir(parent.c_str(), 0777);
#endif
            }
        }
#ifdef _WIN32
        _mkdir(path.c_str());
        struct _stat st;
        return (_stat(path.c_str(), &st) == 0 && (st.st_mode & _S_IFDIR));
#else
        mkdir(path.c_str(), 0777);
        struct stat st;
        return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
#endif
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

bool GameExporter::CopyFile(const std::string& source, const std::string& destination) {
    try {
        std::ifstream src(source, std::ios::binary);
        if (!src.is_open()) {
            std::cout << "Source file not found: " << source << std::endl;
            return false;
        }
        
        std::ofstream dst(destination, std::ios::binary);
        if (!dst.is_open()) {
            std::cerr << "Could not create destination file: " << destination << std::endl;
            return false;
        }
        
        dst << src.rdbuf();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error copying file from " << source << " to " << destination << ": " << e.what() << std::endl;
        return false;
    }
}

bool GameExporter::WriteTextFile(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path);
        if (file.is_open()) {
            file << content;
            file.close();
            return true;
        } else {
            std::cerr << "Could not create file: " << path << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error writing file " << path << ": " << e.what() << std::endl;
        return false;
    }
}

void GameExporter::UpdateExportProgress(float progress, const std::string& status) {
    m_exportProgress = progress;
    m_exportStatus = status;
    std::cout << "[Export] " << progress << "% - " << status << std::endl;
}