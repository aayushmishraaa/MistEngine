#include "AI/AIConfig.h"
#include "AI/SimpleJson.h"
#include <fstream>
#include <iostream>

AIConfig& AIConfig::Instance() {
    static AIConfig instance;
    return instance;
}

bool AIConfig::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Create default config if file doesn't exist
        return SaveToFile(filename);
    }
    
    // For now, just create a default configuration
    // In a full implementation, you would parse the JSON file
    std::cerr << "Config file loading not fully implemented. Using defaults." << std::endl;
    return true;
}

bool AIConfig::SaveToFile(const std::string& filename) {
    try {
        SimpleJson j;
        j.SetObject();
        
        // Save API keys (but not the actual keys for security)
        SimpleJson apiKeys;
        apiKeys.SetObject();
        for (const auto& pair : m_apiKeys) {
            apiKeys[pair.first] = SimpleJson(pair.second.empty() ? "" : "***CONFIGURED***");
        }
        j["api_keys"] = apiKeys;
        
        // Save endpoints
        SimpleJson endpoints;
        endpoints.SetObject();
        for (const auto& pair : m_endpoints) {
            endpoints[pair.first] = SimpleJson(pair.second);
        }
        j["endpoints"] = endpoints;
        
        // Save defaults
        SimpleJson defaults;
        defaults.SetObject();
        defaults["model"] = SimpleJson(m_defaultModel);
        defaults["temperature"] = SimpleJson(m_defaultTemperature);
        defaults["max_tokens"] = SimpleJson(static_cast<double>(m_defaultMaxTokens));
        j["defaults"] = defaults;
        
        // Add configuration instructions
        SimpleJson instructions;
        instructions.SetObject();
        instructions["description"] = SimpleJson("MistEngine AI Configuration");
        instructions["note"] = SimpleJson("Replace api_keys with actual API keys. This file should not be committed to version control.");
        instructions["example_openai_key"] = SimpleJson("sk-your-openai-api-key-here");
        instructions["example_azure_endpoint"] = SimpleJson("https://your-resource.openai.azure.com/");
        j["_instructions"] = instructions;
        
        std::ofstream file(filename);
        file << j.Dump(4);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save AI config file: " << e.what() << std::endl;
        return false;
    }
}

void AIConfig::SetAPIKey(const std::string& provider, const std::string& apiKey) {
    m_apiKeys[provider] = apiKey;
}

std::string AIConfig::GetAPIKey(const std::string& provider) const {
    auto it = m_apiKeys.find(provider);
    return (it != m_apiKeys.end()) ? it->second : "";
}

bool AIConfig::HasAPIKey(const std::string& provider) const {
    auto it = m_apiKeys.find(provider);
    return (it != m_apiKeys.end()) && !it->second.empty();
}

void AIConfig::SetEndpoint(const std::string& provider, const std::string& endpoint) {
    m_endpoints[provider] = endpoint;
}

std::string AIConfig::GetEndpoint(const std::string& provider) const {
    auto it = m_endpoints.find(provider);
    return (it != m_endpoints.end()) ? it->second : "";
}

void AIConfig::SetDefaultModel(const std::string& model) {
    m_defaultModel = model;
}

std::string AIConfig::GetDefaultModel() const {
    return m_defaultModel;
}

void AIConfig::SetDefaultTemperature(float temperature) {
    m_defaultTemperature = std::max(0.0f, std::min(2.0f, temperature));
}

float AIConfig::GetDefaultTemperature() const {
    return m_defaultTemperature;
}

void AIConfig::SetDefaultMaxTokens(int maxTokens) {
    m_defaultMaxTokens = std::max(1, std::min(4000, maxTokens));
}

int AIConfig::GetDefaultMaxTokens() const {
    return m_defaultMaxTokens;
}