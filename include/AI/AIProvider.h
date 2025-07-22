#ifndef AIPROVIDER_H
#define AIPROVIDER_H

#include <string>
#include <vector>
#include <functional>
#include <future>

struct AIMessage {
    enum Role {
        USER,
        ASSISTANT,
        SYSTEM
    };
    
    Role role;
    std::string content;
    
    AIMessage(Role r, const std::string& c) : role(r), content(c) {}
};

struct AIRequest {
    std::vector<AIMessage> messages;
    std::string model = "gpt-3.5-turbo";
    float temperature = 0.7f;
    int maxTokens = 1000;
    std::string systemPrompt;
};

struct AIResponse {
    bool success;
    std::string content;
    std::string errorMessage;
    int tokensUsed = 0;
    
    AIResponse() : success(false), tokensUsed(0) {}
    AIResponse(const std::string& response) : success(true), content(response), tokensUsed(0) {}
    AIResponse(bool success, const std::string& error) : success(success), errorMessage(error), tokensUsed(0) {}
};

class AIProvider {
public:
    virtual ~AIProvider() = default;
    
    // Synchronous request
    virtual AIResponse SendRequest(const AIRequest& request) = 0;
    
    // Asynchronous request
    virtual std::future<AIResponse> SendRequestAsync(const AIRequest& request) = 0;
    
    // Configuration
    virtual bool Initialize(const std::string& apiKey, const std::string& endpoint = "") = 0;
    virtual bool IsInitialized() const = 0;
    virtual std::string GetProviderName() const = 0;
    
    // Utility methods
    virtual std::vector<std::string> GetAvailableModels() const = 0;
};

#endif // AIPROVIDER_H