#include "AI/AIWindow.h"
#include "AI/AIManager.h"
#include "AI/AIProvider.h" // Add this include for AIResponse
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <chrono>

ChatMessage::ChatMessage(Type t, const std::string& c) : type(t), content(c) {
    auto now = std::time(nullptr);
    struct tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localTime = *std::localtime(&now);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%H:%M:%S");
    timestamp = oss.str();
}

AIWindow::AIWindow() 
    : m_visible(false)
    , m_aiManager(nullptr)
    , m_autoScroll(true)
    , m_showSettings(false)
    , m_requestInProgress(false)
    , m_selectedRequestType(0)
    , m_useConversationMode(false) {
    
    memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
    memset(m_customSystemPrompt, 0, sizeof(m_customSystemPrompt));
    
    // Add welcome message
    AddMessage(ChatMessage::SYSTEM, "Welcome to MistEngine AI Assistant! Ask me anything about game development, features, or code implementation.");
}

AIWindow::~AIWindow() = default;

void AIWindow::Draw() {
    if (!m_visible) return;
    
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Ask AI", &m_visible)) {
        // Check for pending responses
        ProcessPendingResponse();
        
        // Settings button
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        if (ImGui::Button("Settings")) {
            m_showSettings = !m_showSettings;
        }
        
        // Settings panel
        if (m_showSettings) {
            DrawSettingsPanel();
            ImGui::Separator();
        }
        
        // Request type selector
        DrawRequestTypeSelector();
        ImGui::Separator();
        
        // Chat area (takes most of the space)
        DrawChatArea();
        
        ImGui::Separator();
        
        // Input area at the bottom
        DrawInputArea();
    }
    ImGui::End();
}

void AIWindow::SetAIManager(AIManager* aiManager) {
    m_aiManager = aiManager;
    
    if (m_aiManager && m_aiManager->HasActiveProvider()) {
        AddMessage(ChatMessage::SYSTEM, "AI provider connected: " + m_aiManager->GetActiveProviderName());
    } else {
        AddMessage(ChatMessage::ERROR, "No AI provider available. Please configure an API key in settings.");
    }
}

void AIWindow::AddMessage(ChatMessage::Type type, const std::string& content) {
    m_chatHistory.emplace_back(type, content);
    
    // Limit chat history to prevent memory issues
    if (m_chatHistory.size() > 1000) {
        m_chatHistory.erase(m_chatHistory.begin());
    }
}

void AIWindow::ClearChat() {
    m_chatHistory.clear();
    if (m_aiManager) {
        m_aiManager->ClearHistory();
    }
    AddMessage(ChatMessage::SYSTEM, "Chat cleared. Starting new conversation.");
}

void AIWindow::DrawChatArea() {
    ImGui::BeginChild("ChatArea", ImVec2(0, -70), true);
    
    for (const auto& message : m_chatHistory) {
        ImVec4 color = GetMessageColor(message.type);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        
        // Message header
        std::string prefix;
        switch (message.type) {
            case ChatMessage::USER: prefix = "[You]"; break;
            case ChatMessage::ASSISTANT: prefix = "[AI]"; break;
            case ChatMessage::SYSTEM: prefix = "[System]"; break;
            case ChatMessage::ERROR: prefix = "[Error]"; break;
        }
        
        ImGui::Text("%s %s", message.timestamp.c_str(), prefix.c_str());
        ImGui::PopStyleColor();
        
        // Message content
        ImGui::TextWrapped("%s", message.content.c_str());
        ImGui::Spacing();
    }
    
    // Auto-scroll to bottom
    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
}

void AIWindow::DrawInputArea() {
    // Input field with proper clipboard support
    ImGui::PushItemWidth(-120);
    
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue |
                                   ImGuiInputTextFlags_CtrlEnterForNewLine |
                                   ImGuiInputTextFlags_AllowTabInput;
    
    bool enterPressed = ImGui::InputTextMultiline("##Input", m_inputBuffer, sizeof(m_inputBuffer), 
                                                 ImVec2(0, 50), inputFlags);
    ImGui::PopItemWidth();
    
    // Show tooltip for keyboard shortcuts
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enter: Send message\nCtrl+Enter: New line\nCtrl+V: Paste\nCtrl+A: Select all");
    }
    
    ImGui::SameLine();
    
    // Send button
    bool canSend = strlen(m_inputBuffer) > 0 && !m_requestInProgress && m_aiManager && m_aiManager->HasActiveProvider();
    
    if (!canSend) {
        ImGui::BeginDisabled();
    }
    
    bool sendClicked = ImGui::Button("Send", ImVec2(100, 50));
    
    if (!canSend) {
        ImGui::EndDisabled();
    }
    
    // Additional controls
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::SameLine();
    ImGui::Checkbox("Conversation mode", &m_useConversationMode);
    ImGui::SameLine();
    if (ImGui::Button("Clear Chat")) {
        ClearChat();
    }
    
    // Send message if enter pressed or send button clicked
    if ((enterPressed || sendClicked) && canSend) {
        SendMessage();
    }
    
    // Show status
    if (m_requestInProgress) {
        ImGui::Text("AI is thinking...");
    } else if (!m_aiManager || !m_aiManager->HasActiveProvider()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No AI provider configured");
    }
}

void AIWindow::DrawSettingsPanel() {
    if (ImGui::CollapsingHeader("AI Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (m_aiManager) {
            // Model selection
            if (m_aiManager->HasActiveProvider()) {
                std::string currentModel = m_aiManager->GetModel();
                if (ImGui::BeginCombo("Model", currentModel.c_str())) {
                    std::vector<std::string> models = {"gpt-3.5-turbo", "gpt-4", "gpt-4-turbo-preview"};
                    for (const auto& model : models) {
                        bool isSelected = (currentModel == model);
                        if (ImGui::Selectable(model.c_str(), isSelected)) {
                            m_aiManager->SetModel(model);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                
                // Temperature slider
                float temperature = m_aiManager->GetTemperature();
                if (ImGui::SliderFloat("Temperature", &temperature, 0.0f, 2.0f, "%.2f")) {
                    m_aiManager->SetTemperature(temperature);
                }
                
                // Max tokens
                int maxTokens = m_aiManager->GetMaxTokens();
                if (ImGui::SliderInt("Max Tokens", &maxTokens, 100, 4000)) {
                    m_aiManager->SetMaxTokens(maxTokens);
                }
            } else {
                ImGui::Text("Provider Status: Not connected");
                ImGui::Text("Configure API key to enable AI features");
            }
        }
        
        // Custom system prompt
        if (m_selectedRequestType == CUSTOM) {
            ImGui::Text("Custom System Prompt:");
            ImGui::InputTextMultiline("##SystemPrompt", m_customSystemPrompt, sizeof(m_customSystemPrompt), ImVec2(-1, 100));
        }
    }
}

void AIWindow::DrawRequestTypeSelector() {
    const char* requestTypes[] = {
        "General Chat",
        "Feature Suggestion",
        "Code Implementation",
        "Game Logic Advice",
        "Code Optimization",
        "Code Explanation",
        "Custom"
    };
    
    ImGui::Text("Request Type:");
    ImGui::SameLine();
    ImGui::Combo("##RequestType", &m_selectedRequestType, requestTypes, IM_ARRAYSIZE(requestTypes));
    
    // Show description based on selected type
    switch (m_selectedRequestType) {
        case GENERAL_CHAT:
            ImGui::TextWrapped("General conversation about game development topics.");
            break;
        case FEATURE_SUGGESTION:
            ImGui::TextWrapped("Get suggestions for new game engine features and implementations.");
            break;
        case CODE_IMPLEMENTATION:
            ImGui::TextWrapped("Request specific code implementations for game features.");
            break;
        case GAME_LOGIC_ADVICE:
            ImGui::TextWrapped("Get advice on game logic design and implementation patterns.");
            break;
        case CODE_OPTIMIZATION:
            ImGui::TextWrapped("Get suggestions for optimizing existing code.");
            break;
        case CODE_EXPLANATION:
            ImGui::TextWrapped("Get explanations of how code works and its purpose.");
            break;
        case CUSTOM:
            ImGui::TextWrapped("Use a custom system prompt for specialized requests.");
            break;
    }
}

void AIWindow::SendMessage() {
    if (!m_aiManager || !m_aiManager->HasActiveProvider()) {
        AddMessage(ChatMessage::ERROR, "No AI provider available");
        return;
    }
    
    std::string message = m_inputBuffer;
    if (message.empty()) return;
    
    // Add user message to chat
    AddMessage(ChatMessage::USER, message);
    
    // Clear input
    memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
    
    // Send request based on type
    m_requestInProgress = true;
    
    std::string systemPrompt = GetRequestTypeSystemPrompt();
    
    if (m_useConversationMode) {
        // Add to conversation history
        if (m_aiManager->GetConversationHistory().empty()) {
            m_aiManager->StartNewConversation();
        }
        m_pendingRequest = m_aiManager->SendRequestAsync(message, systemPrompt);
    } else {
        // Send specialized request based on type
        switch (m_selectedRequestType) {
            case FEATURE_SUGGESTION:
                m_pendingRequest = std::async(std::launch::async, [this, message]() {
                    return m_aiManager->GetFeatureSuggestion(message);
                });
                break;
            case CODE_IMPLEMENTATION:
                m_pendingRequest = std::async(std::launch::async, [this, message]() {
                    return m_aiManager->GetCodeImplementation(message);
                });
                break;
            case GAME_LOGIC_ADVICE:
                m_pendingRequest = std::async(std::launch::async, [this, message]() {
                    return m_aiManager->GetGameLogicAdvice(message);
                });
                break;
            case CODE_OPTIMIZATION:
            case CODE_EXPLANATION:
            default:
                m_pendingRequest = m_aiManager->SendRequestAsync(message, systemPrompt);
                break;
        }
    }
}

void AIWindow::ProcessPendingResponse() {
    if (m_requestInProgress && m_pendingRequest.valid()) {
        if (m_pendingRequest.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            AIResponse response = m_pendingRequest.get();
            HandleAIResponse(response);
            m_requestInProgress = false;
        }
    }
}

void AIWindow::HandleAIResponse(const AIResponse& response) {
    if (response.success) {
        AddMessage(ChatMessage::ASSISTANT, response.content);
        
        if (m_useConversationMode && m_aiManager) {
            // Add to conversation history for context
            std::string lastUserMessage = "";
            if (!m_chatHistory.empty() && m_chatHistory.back().type == ChatMessage::ASSISTANT) {
                // Find the last user message
                for (auto it = m_chatHistory.rbegin(); it != m_chatHistory.rend(); ++it) {
                    if (it->type == ChatMessage::USER) {
                        lastUserMessage = it->content;
                        break;
                    }
                }
            }
            
            if (!lastUserMessage.empty()) {
                m_aiManager->AddToConversation(lastUserMessage, response.content);
            }
        }
        
        if (response.tokensUsed > 0) {
            AddMessage(ChatMessage::SYSTEM, "Tokens used: " + std::to_string(response.tokensUsed));
        }
    } else {
        AddMessage(ChatMessage::ERROR, "AI request failed: " + response.errorMessage);
    }
}

std::string AIWindow::GetCurrentTimestamp() {
    auto now = std::time(nullptr);
    struct tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localTime = *std::localtime(&now);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%H:%M:%S");
    return oss.str();
}

ImVec4 AIWindow::GetMessageColor(ChatMessage::Type type) {
    switch (type) {
        case ChatMessage::USER: return ImVec4(0.7f, 0.9f, 1.0f, 1.0f);       // Light blue
        case ChatMessage::ASSISTANT: return ImVec4(0.9f, 1.0f, 0.9f, 1.0f);  // Light green
        case ChatMessage::SYSTEM: return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);     // Gray
        case ChatMessage::ERROR: return ImVec4(1.0f, 0.6f, 0.6f, 1.0f);      // Light red
        default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);                     // White
    }
}

std::string AIWindow::GetRequestTypeSystemPrompt() {
    if (m_selectedRequestType == CUSTOM) {
        return std::string(m_customSystemPrompt);
    }
    
    // Return appropriate system prompt based on request type
    // This will be handled by AIManager's specialized methods
    return "";
}

const char* AIWindow::GetRequestTypeName(RequestType type) {
    switch (type) {
        case GENERAL_CHAT: return "General Chat";
        case FEATURE_SUGGESTION: return "Feature Suggestion";
        case CODE_IMPLEMENTATION: return "Code Implementation";
        case GAME_LOGIC_ADVICE: return "Game Logic Advice";
        case CODE_OPTIMIZATION: return "Code Optimization";
        case CODE_EXPLANATION: return "Code Explanation";
        case CUSTOM: return "Custom";
        default: return "Unknown";
    }
}