#include "AI/AIManager.h"
#include "AI/GeminiProvider.h"
#include <iostream>

AIManager::AIManager() 
    : m_currentModel("gemini-1.5-flash")
    , m_temperature(0.7f)
    , m_maxTokens(1000) {
}

AIManager::~AIManager() = default;

bool AIManager::InitializeProvider(const std::string& providerName, const std::string& apiKey, const std::string& endpoint) {
    if (providerName == "Gemini" || providerName == "Google Gemini" || providerName == "gemini") {
        m_provider = std::make_unique<GeminiProvider>();
        return m_provider->Initialize(apiKey, endpoint);
    }
    
    std::cerr << "Unknown AI provider: " << providerName << std::endl;
    return false;
}

bool AIManager::HasActiveProvider() const {
    return m_provider && m_provider->IsInitialized();
}

std::string AIManager::GetActiveProviderName() const {
    if (HasActiveProvider()) {
        return m_provider->GetProviderName();
    }
    return "None";
}

AIResponse AIManager::SendRequest(const std::string& prompt, const std::string& systemPrompt) {
    if (!HasActiveProvider()) {
        return AIResponse(false, "No AI provider available");
    }
    
    AIRequest request = BuildRequest(prompt, systemPrompt);
    return m_provider->SendRequest(request);
}

std::future<AIResponse> AIManager::SendRequestAsync(const std::string& prompt, const std::string& systemPrompt) {
    if (!HasActiveProvider()) {
        // Return a future with an error response
        std::promise<AIResponse> promise;
        promise.set_value(AIResponse(false, "No AI provider available"));
        return promise.get_future();
    }
    
    AIRequest request = BuildRequest(prompt, systemPrompt);
    return m_provider->SendRequestAsync(request);
}

AIResponse AIManager::GetFeatureSuggestion(const std::string& description) {
    std::string systemPrompt = GetFeatureSystemPrompt();
    std::string prompt = "I'm working on a game engine feature and need suggestions. Here's the description: " + description;
    
    return SendRequest(prompt, systemPrompt);
}

AIResponse AIManager::GetCodeImplementation(const std::string& requirement, const std::string& language) {
    std::string systemPrompt = GetCodeAnalysisSystemPrompt();
    std::string prompt = "Please provide a " + language + " implementation for the following requirement: " + requirement;
    
    return SendRequest(prompt, systemPrompt);
}

AIResponse AIManager::GetGameLogicAdvice(const std::string& scenario) {
    std::string systemPrompt = GetGameDevSystemPrompt();
    std::string prompt = "I need advice on implementing game logic for this scenario: " + scenario;
    
    return SendRequest(prompt, systemPrompt);
}

AIResponse AIManager::OptimizeCode(const std::string& code) {
    std::string systemPrompt = GetCodeAnalysisSystemPrompt();
    std::string prompt = "Please analyze and suggest optimizations for this code:\n\n```cpp\n" + code + "\n```";
    
    return SendRequest(prompt, systemPrompt);
}

AIResponse AIManager::ExplainCode(const std::string& code) {
    std::string systemPrompt = GetCodeAnalysisSystemPrompt();
    std::string prompt = "Please explain what this code does and how it works:\n\n```cpp\n" + code + "\n```";
    
    return SendRequest(prompt, systemPrompt);
}

void AIManager::StartNewConversation() {
    m_conversationHistory.clear();
}

void AIManager::AddToConversation(const std::string& userMessage, const std::string& assistantResponse) {
    m_conversationHistory.emplace_back(AIMessage::USER, userMessage);
    m_conversationHistory.emplace_back(AIMessage::ASSISTANT, assistantResponse);
}

std::vector<AIMessage> AIManager::GetConversationHistory() const {
    return m_conversationHistory;
}

void AIManager::ClearHistory() {
    m_conversationHistory.clear();
}

void AIManager::SetModel(const std::string& model) {
    m_currentModel = model;
}

void AIManager::SetTemperature(float temperature) {
    m_temperature = std::max(0.0f, std::min(2.0f, temperature));
}

void AIManager::SetMaxTokens(int maxTokens) {
    m_maxTokens = std::max(1, std::min(4000, maxTokens));
}

std::vector<std::string> AIManager::GetAvailableModels() const {
    if (HasActiveProvider()) {
        return m_provider->GetAvailableModels();
    }
    return {}; // Return empty vector if no provider is active
}

std::string AIManager::GetGameDevSystemPrompt() const {
    return "You are an expert game developer and AI assistant specialized in game engine development, "
           "game logic implementation, and C++ programming. You have extensive knowledge of graphics programming, "
           "physics systems, ECS architecture, and modern game development practices. "
           "Provide practical, optimized solutions with clear explanations. "
           "Focus on performance, maintainability, and best practices in game development.";
}

std::string AIManager::GetCodeAnalysisSystemPrompt() const {
    return "You are an expert C++ developer and code analyst with deep knowledge of game engine architecture, "
           "performance optimization, and modern C++ best practices. "
           "When analyzing code, focus on correctness, performance, readability, and maintainability. "
           "Provide specific, actionable suggestions with code examples when appropriate. "
           "Consider memory management, CPU/GPU performance, and scalability in your recommendations.";
}

std::string AIManager::GetFeatureSystemPrompt() const {
    return "You are a game engine architect and technical lead with expertise in designing scalable, "
           "modular game engine features. You understand the technical requirements and constraints "
           "of real-time game engines, including performance, memory usage, and architectural patterns. "
           "When suggesting features, consider implementation complexity, integration with existing systems, "
           "and long-term maintainability. Provide detailed technical specifications and implementation strategies.";
}

AIRequest AIManager::BuildRequest(const std::string& prompt, const std::string& systemPrompt) {
    AIRequest request;
    request.model = m_currentModel;
    request.temperature = m_temperature;
    request.maxTokens = m_maxTokens;
    request.systemPrompt = systemPrompt;
    
    request.messages.emplace_back(AIMessage::USER, prompt);
    
    return request;
}

AIRequest AIManager::BuildConversationRequest(const std::string& prompt, const std::string& systemPrompt) {
    AIRequest request;
    request.model = m_currentModel;
    request.temperature = m_temperature;
    request.maxTokens = m_maxTokens;
    request.systemPrompt = systemPrompt;
    
    // Add conversation history
    request.messages = m_conversationHistory;
    
    // Add new user message
    request.messages.emplace_back(AIMessage::USER, prompt);
    
    return request;
}

AIResponse AIManager::TestConnection() {
    if (!HasActiveProvider()) {
        return AIResponse(false, "No AI provider available");
    }
    
    // Send a simple test request with minimal token usage
    AIRequest request;
    request.model = "gemini-1.5-flash"; // Use the available Gemini model for testing
    request.temperature = 0.1f; // Low temperature for consistent response
    request.maxTokens = 20; // Very small response to minimize usage
    request.systemPrompt = "You are a helpful AI assistant. Respond very briefly.";
    
    request.messages.emplace_back(AIMessage::USER, "Hello, please respond with 'Connection successful!'");
    
    AIResponse testResponse = m_provider->SendRequest(request);
    
    if (testResponse.success) {
        testResponse.content = "? Gemini connection successful! " + testResponse.content;
        testResponse.errorMessage = "Google Gemini API connection verified";
    } else {
        // Add additional context to error message
        std::string enhancedError = "? Gemini connection failed: " + testResponse.errorMessage;
        
        if (testResponse.errorMessage.find("401") != std::string::npos) {
            enhancedError += "\n\n?? API KEY ISSUES:\n";
            enhancedError += "• Your API key appears to be invalid\n";
            enhancedError += "• Get a key from https://aistudio.google.com/app/apikey\n";
            enhancedError += "• Make sure you copied the entire key\n";
            enhancedError += "• Ensure the key is active";
        } else if (testResponse.errorMessage.find("403") != std::string::npos) {
            enhancedError += "\n\n?? ACCESS ISSUES:\n";
            enhancedError += "• Gemini API may not be enabled\n";
            enhancedError += "• Check your Google Cloud project settings\n";
            enhancedError += "• Verify API access permissions\n";
            enhancedError += "• Try creating a new API key";
        } else if (testResponse.errorMessage.find("404") != std::string::npos) {
            enhancedError += "\n\n?? MODEL NOT FOUND:\n";
            enhancedError += "• Using updated model: gemini-1.5-flash\n";
            enhancedError += "• Older model names may not be available\n";
            enhancedError += "• API version updated to v1\n";
            enhancedError += "• Try the test again with the updated model";
        } else if (testResponse.errorMessage.find("429") != std::string::npos) {
            enhancedError += "\n\n?? RATE LIMIT:\n";
            enhancedError += "• Free tier: 15 requests/minute\n";
            enhancedError += "• Wait a moment and try again\n";
            enhancedError += "• Consider upgrading for higher limits";
        }
        
        testResponse.errorMessage = enhancedError;
    }
    
    return testResponse;
}

std::future<AIResponse> AIManager::TestConnectionAsync() {
    return std::async(std::launch::async, [this]() {
        return TestConnection();
    });
}