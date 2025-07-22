#ifndef AICONFIG_H
#define AICONFIG_H

#include <string>
#include <map>

class AIConfig {
public:
    static AIConfig& Instance();
    
    // Configuration loading/saving
    bool LoadFromFile(const std::string& filename = "ai_config.json");
    bool SaveToFile(const std::string& filename = "ai_config.json");
    
    // API Key management
    void SetAPIKey(const std::string& provider, const std::string& apiKey);
    std::string GetAPIKey(const std::string& provider) const;
    bool HasAPIKey(const std::string& provider) const;
    
    // Endpoint management
    void SetEndpoint(const std::string& provider, const std::string& endpoint);
    std::string GetEndpoint(const std::string& provider) const;
    
    // Default settings
    void SetDefaultModel(const std::string& model);
    std::string GetDefaultModel() const;
    
    void SetDefaultTemperature(float temperature);
    float GetDefaultTemperature() const;
    
    void SetDefaultMaxTokens(int maxTokens);
    int GetDefaultMaxTokens() const;
    
private:
    AIConfig() = default;
    ~AIConfig() = default;
    AIConfig(const AIConfig&) = delete;
    AIConfig& operator=(const AIConfig&) = delete;
    
    std::map<std::string, std::string> m_apiKeys;
    std::map<std::string, std::string> m_endpoints;
    std::string m_defaultModel = "gemini-1.5-flash";
    float m_defaultTemperature = 0.7f;
    int m_defaultMaxTokens = 1000;
};

#endif // AICONFIG_H