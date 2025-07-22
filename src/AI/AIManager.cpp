#include "AI/AIManager.h"
#include "AI/OpenAIProvider.h"
#include <iostream>

AIManager::AIManager() 
    : m_currentModel("gpt-3.5-turbo")
    , m_temperature(0.7f)
    , m_maxTokens(1000) {
}

AIManager::~AIManager() = default;

bool AIManager::InitializeProvider(const std::string& providerName, const std::string& apiKey, const std::string& endpoint) {
    if (providerName == "OpenAI" || providerName == "openai") {
        m_provider = std::make_unique<OpenAIProvider>();
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
    
    // Send a simple test request
    AIRequest request;
    request.model = m_currentModel;
    request.temperature = 0.1f; // Low temperature for consistent response
    request.maxTokens = 50; // Small response to save tokens
    request.systemPrompt = "You are a helpful assistant. Respond briefly.";
    
    request.messages.emplace_back(AIMessage::USER, "Hello, can you confirm you're working? Please respond with just 'Connection successful.'");
    
    return m_provider->SendRequest(request);
}

std::future<AIResponse> AIManager::TestConnectionAsync() {
    return std::async(std::launch::async, [this]() {
        return TestConnection();
    });
}