#include "GameExporter.h"
#include "Version.h"  // Add Version.h include
#include <iostream>
#include <fstream>
#include <sstream>
#include <direct.h>  // For _mkdir on Windows
#include <sys/stat.h>  // For checking if directory exists

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
    
    // Create output directory
    std::string outputPath = settings.outputDirectory + "/" + settings.gameName;
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
    return {
        "MistEngine.exe",  // Main executable
        "glfw3.dll",       // GLFW library
        "opengl32.dll",    // OpenGL library
        "shaders/",        // Shader files
        "textures/",       // Texture files
        "models/",         // Model files (if any)
    };
}

bool GameExporter::CopyEngineFiles(const std::string& outputPath) {
    // This is a simplified implementation
    // In a real exporter, you would copy the actual engine executable and dependencies
    
    std::cout << "Copying engine files to: " << outputPath << std::endl;
    
    // Create subdirectories
    CreateDirectory(outputPath + "/shaders");
    CreateDirectory(outputPath + "/textures");
    CreateDirectory(outputPath + "/assets");
    
    // Copy shader files
    if (!CopyFile("shaders/object.vert", outputPath + "/shaders/object.vert")) {
        std::cout << "Warning: Could not copy vertex shader" << std::endl;
    }
    if (!CopyFile("shaders/object.frag", outputPath + "/shaders/object.frag")) {
        std::cout << "Warning: Could not copy fragment shader" << std::endl;
    }
    
    // Note: In a real implementation, you would copy the compiled executable
    // For this demo, we create a placeholder
    std::string placeholderContent = "# MistFPS Game Executable Placeholder\n";
    placeholderContent += "# In a real export, this would be the compiled game executable\n";
    placeholderContent += "# Built with " + std::string(MIST_ENGINE_NAME) + " " + std::string(MIST_ENGINE_VERSION_STRING) + "\n";
    placeholderContent += "# Platform: " + std::string(MIST_ENGINE_PLATFORM) + "\n";
    placeholderContent += "# Compiler: " + std::string(MIST_ENGINE_COMPILER) + "\n";
    
    return WriteTextFile(outputPath + "/MistFPS.exe.txt", placeholderContent);
}

bool GameExporter::CopyGameAssets(const std::string& outputPath) {
    std::cout << "Copying game assets to: " << outputPath << "/assets" << std::endl;
    
    // Create assets directory
    CreateDirectory(outputPath + "/assets");
    CreateDirectory(outputPath + "/assets/models");
    CreateDirectory(outputPath + "/assets/textures");
    CreateDirectory(outputPath + "/assets/sounds");
    
    // In a real implementation, you would copy actual asset files
    // For this demo, create placeholder files
    WriteTextFile(outputPath + "/assets/models/player.obj", "# Player model placeholder");
    WriteTextFile(outputPath + "/assets/models/enemy.obj", "# Enemy model placeholder");
    WriteTextFile(outputPath + "/assets/models/weapons.obj", "# Weapons model placeholder");
    
    WriteTextFile(outputPath + "/assets/textures/player.png", "# Player texture placeholder");
    WriteTextFile(outputPath + "/assets/textures/enemy.png", "# Enemy texture placeholder");
    WriteTextFile(outputPath + "/assets/textures/weapons.png", "# Weapons texture placeholder");
    
    WriteTextFile(outputPath + "/assets/sounds/gunshot.wav", "# Gunshot sound placeholder");
    WriteTextFile(outputPath + "/assets/sounds/reload.wav", "# Reload sound placeholder");
    WriteTextFile(outputPath + "/assets/sounds/enemy_death.wav", "# Enemy death sound placeholder");
    
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
    // In a real implementation, this would create an actual executable
    // For this demo, create a batch file launcher
    
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
    launcher << "REM In a real export, this would launch MistFPS.exe\n";
    launcher << "echo Game would start here with MistFPS.exe\n";
    launcher << "pause\n";
    
    return WriteTextFile(outputPath + "/Launch_" + settings.gameName + ".bat", launcher.str());
}

bool GameExporter::CreateReadme(const std::string& outputPath, const ExportSettings& settings) {
    std::ostringstream readme;
    readme << "# " << settings.gameName << " v" << settings.version << "\n\n";
    readme << "A first-person shooter game built with " << MIST_ENGINE_NAME << " " << MIST_ENGINE_VERSION_STRING << ".\n\n";
    readme << "## System Requirements\n";
    readme << "- Windows 10 or later\n";
    readme << "- OpenGL 3.3 compatible graphics card\n";
    readme << "- 2GB RAM minimum\n";
    readme << "- 500MB disk space\n\n";
    readme << "## How to Play\n";
    readme << "1. Run Launch_" << settings.gameName << ".bat to start the game\n";
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
#ifdef _WIN32
        if (_mkdir(path.c_str()) == 0) {
            return true;
        }
        // Directory might already exist
        struct _stat st;
        if (_stat(path.c_str(), &st) == 0 && (st.st_mode & _S_IFDIR)) {
            return true; // Directory already exists
        }
#else
        if (mkdir(path.c_str(), 0777) == 0) {
            return true;
        }
        struct stat st;
        if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            return true; // Directory already exists
        }
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