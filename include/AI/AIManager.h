#ifndef AIMANAGER_H
#define AIMANAGER_H

#include "AI/AIProvider.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

class AIManager {
public:
    AIManager();
    ~AIManager();
    
    // Provider management
    bool InitializeProvider(const std::string& providerName, const std::string& apiKey, const std::string& endpoint = "");
    bool HasActiveProvider() const;
    std::string GetActiveProviderName() const;
    
    // Connection testing
    AIResponse TestConnection();
    std::future<AIResponse> TestConnectionAsync();
    
    // AI Request methods
    AIResponse SendRequest(const std::string& prompt, const std::string& systemPrompt = "");
    std::future<AIResponse> SendRequestAsync(const std::string& prompt, const std::string& systemPrompt = "");
    
    // Specialized requests for game development
    AIResponse GetFeatureSuggestion(const std::string& description);
    AIResponse GetCodeImplementation(const std::string& requirement, const std::string& language = "C++");
    AIResponse GetGameLogicAdvice(const std::string& scenario);
    AIResponse OptimizeCode(const std::string& code);
    AIResponse ExplainCode(const std::string& code);
    
    // Chat history management
    void StartNewConversation();
    void AddToConversation(const std::string& userMessage, const std::string& assistantResponse);
    std::vector<AIMessage> GetConversationHistory() const;
    void ClearHistory();
    
    // Configuration
    void SetModel(const std::string& model);
    void SetTemperature(float temperature);
    void SetMaxTokens(int maxTokens);
    
    std::string GetModel() const { return m_currentModel; }
    float GetTemperature() const { return m_temperature; }
    int GetMaxTokens() const { return m_maxTokens; }
    std::vector<std::string> GetAvailableModels() const;
    
private:
    std::unique_ptr<AIProvider> m_provider;
    std::vector<AIMessage> m_conversationHistory;
    
    // Default settings
    std::string m_currentModel;
    float m_temperature;
    int m_maxTokens;
    
    // System prompts for different use cases
    std::string GetGameDevSystemPrompt() const;
    std::string GetCodeAnalysisSystemPrompt() const;
    std::string GetFeatureSystemPrompt() const;
    
    // Helper methods
    AIRequest BuildRequest(const std::string& prompt, const std::string& systemPrompt = "");
    AIRequest BuildConversationRequest(const std::string& prompt, const std::string& systemPrompt = "");
};

#endif // AIMANAGER_H