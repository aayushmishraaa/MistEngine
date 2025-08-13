#ifndef GAMEEXPORTER_H
#define GAMEEXPORTER_H

#include <string>
#include <vector>

struct ExportSettings {
    std::string outputDirectory = "exports";
    std::string gameName = "MistFPS";
    std::string version = "1.0";
    bool includeAssets = true;
    bool createInstaller = false;
    bool compressAssets = true;
    
    // Level settings
    int numberOfLevels = 5;
    int enemiesPerLevel = 10;
    std::vector<std::string> weaponTypes = {"Pistol", "Rifle", "Shotgun", "Sniper"};
};

class GameExporter {
public:
    GameExporter();
    ~GameExporter();
    
    // Export functions
    bool ExportGame(const ExportSettings& settings);
    bool ExportStandalone(const std::string& outputPath);
    bool ExportWithInstaller(const std::string& outputPath);
    
    // Asset packaging
    bool PackageAssets(const std::string& assetsPath);
    bool CompressAssets(const std::string& inputPath, const std::string& outputPath);
    
    // Configuration generation
    bool GenerateGameConfig(const ExportSettings& settings, const std::string& outputPath);
    bool GenerateLevelData(const ExportSettings& settings, const std::string& outputPath);
    
    // Export status
    bool IsExporting() const { return m_isExporting; }
    float GetExportProgress() const { return m_exportProgress; }
    std::string GetExportStatus() const { return m_exportStatus; }
    
    // Validation
    bool ValidateExportSettings(const ExportSettings& settings);
    std::vector<std::string> GetRequiredFiles() const;
    
private:
    bool m_isExporting;
    float m_exportProgress;
    std::string m_exportStatus;
    
    // Helper functions
    bool CopyEngineFiles(const std::string& outputPath);
    bool CopyGameAssets(const std::string& outputPath);
    bool CreateLauncherExecutable(const std::string& outputPath, const ExportSettings& settings);
    bool CreateReadme(const std::string& outputPath, const ExportSettings& settings);
    
    // File operations
    bool CreateDirectory(const std::string& path);
    bool CopyFile(const std::string& source, const std::string& destination);
    bool WriteTextFile(const std::string& path, const std::string& content);
    
    void UpdateExportProgress(float progress, const std::string& status);
};

#endif // GAMEEXPORTER_H