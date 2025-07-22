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
        std::cerr << "Config file not found. Creating default config." << std::endl;
        return SaveToFile(filename);
    }
    
    try {
        // Read the entire file into a string
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        if (content.empty()) {
            std::cerr << "Config file is empty. Creating default config." << std::endl;
            return SaveToFile(filename);
        }
        
        // For now, use a simple parser since we have SimpleJson
        // Look for API keys in the content
        size_t apiKeysPos = content.find("\"api_keys\"");
        if (apiKeysPos != std::string::npos) {
            // Find Gemini key
            size_t geminiPos = content.find("\"Gemini\":", apiKeysPos);
            if (geminiPos == std::string::npos) {
                geminiPos = content.find("\"Google Gemini\":", apiKeysPos);
            }
            
            if (geminiPos != std::string::npos) {
                size_t startQuote = content.find("\"", geminiPos + 15);
                if (startQuote != std::string::npos) {
                    size_t endQuote = content.find("\"", startQuote + 1);
                    if (endQuote != std::string::npos) {
                        std::string apiKey = content.substr(startQuote + 1, endQuote - startQuote - 1);
                        if (apiKey != "***CONFIGURED***" && !apiKey.empty() && apiKey != "true") {
                            m_apiKeys["Gemini"] = apiKey;
                        }
                    }
                }
            }
        }
        
        // Load default model
        size_t modelPos = content.find("\"model\":");
        if (modelPos != std::string::npos) {
            size_t startQuote = content.find("\"", modelPos + 8);
            if (startQuote != std::string::npos) {
                size_t endQuote = content.find("\"", startQuote + 1);
                if (endQuote != std::string::npos) {
                    std::string model = content.substr(startQuote + 1, endQuote - startQuote - 1);
                    if (!model.empty()) {
                        m_defaultModel = model;
                    }
                }
            }
        }
        
        // Load temperature
        size_t tempPos = content.find("\"temperature\":");
        if (tempPos != std::string::npos) {
            size_t colonPos = content.find(":", tempPos);
            if (colonPos != std::string::npos) {
                size_t commaPos = content.find(",", colonPos);
                size_t bracePos = content.find("}", colonPos);
                size_t endPos = (commaPos != std::string::npos && commaPos < bracePos) ? commaPos : bracePos;
                
                if (endPos != std::string::npos) {
                    std::string tempStr = content.substr(colonPos + 1, endPos - colonPos - 1);
                    // Remove whitespace
                    tempStr.erase(0, tempStr.find_first_not_of(" \t\r\n"));
                    tempStr.erase(tempStr.find_last_not_of(" \t\r\n") + 1);
                    
                    try {
                        float temp = std::stof(tempStr);
                        m_defaultTemperature = temp;
                    } catch (...) {
                        // Use default
                    }
                }
            }
        }
        
        // Load max tokens
        size_t tokensPos = content.find("\"max_tokens\":");
        if (tokensPos != std::string::npos) {
            size_t colonPos = content.find(":", tokensPos);
            if (colonPos != std::string::npos) {
                size_t commaPos = content.find(",", colonPos);
                size_t bracePos = content.find("}", colonPos);
                size_t endPos = (commaPos != std::string::npos && commaPos < bracePos) ? commaPos : bracePos;
                
                if (endPos != std::string::npos) {
                    std::string tokensStr = content.substr(colonPos + 1, endPos - colonPos - 1);
                    // Remove whitespace
                    tokensStr.erase(0, tokensStr.find_first_not_of(" \t\r\n"));
                    tokensStr.erase(tokensStr.find_last_not_of(" \t\r\n") + 1);
                    
                    try {
                        int tokens = std::stoi(tokensStr);
                        m_defaultMaxTokens = tokens;
                    } catch (...) {
                        // Use default
                    }
                }
            }
        }
        
        std::cout << "AI configuration loaded successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load AI config file: " << e.what() << std::endl;
        return false;
    }
}

bool AIConfig::SaveToFile(const std::string& filename) {
    try {
        SimpleJson j;
        j.SetObject();
        
        // Save API keys (save actual keys, not masked values)
        SimpleJson apiKeys;
        apiKeys.SetObject();
        for (const auto& pair : m_apiKeys) {
            // Save the actual API key for persistence
            apiKeys[pair.first] = SimpleJson(pair.second);
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
        
        // Add instructions for users
        SimpleJson instructions;
        instructions.SetObject();
        instructions["note"] = SimpleJson("Replace api_keys with actual API keys. This file should not be committed to version control.");
        instructions["example_gemini_key"] = SimpleJson("your-gemini-api-key-here");
        instructions["get_key_from"] = SimpleJson("https://aistudio.google.com/app/apikey");
        j["_instructions"] = instructions;
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << filename << std::endl;
            return false;
        }
        
        file << j.Dump(4);
        file.close();
        
        std::cout << "AI configuration saved to: " << filename << std::endl;
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