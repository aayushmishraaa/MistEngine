#ifndef AIWINDOW_H
#define AIWINDOW_H

#include "imgui.h"
#include "AI/AIProvider.h" // Include for AIResponse definition
#include <string>
#include <vector>
#include <memory>
#include <future>

// Forward declarations
class AIManager;

struct ChatMessage {
    enum Type {
        USER,
        ASSISTANT,
        SYSTEM,
        ERROR
    };
    
    Type type;
    std::string content;
    std::string timestamp;
    
    ChatMessage(Type t, const std::string& c);
};

class AIWindow {
public:
    AIWindow();
    ~AIWindow();
    
    // Main UI methods
    void Draw();
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    void Toggle() { m_visible = !m_visible; }
    
    // AI Manager integration
    void SetAIManager(AIManager* aiManager);
    
    // Chat functionality
    void AddMessage(ChatMessage::Type type, const std::string& content);
    void ClearChat();
    
private:
    bool m_visible;
    AIManager* m_aiManager;
    
    // UI State
    char m_inputBuffer[4096];
    std::vector<ChatMessage> m_chatHistory;
    bool m_autoScroll;
    bool m_showSettings;
    
    // Request state
    bool m_requestInProgress;
    std::future<AIResponse> m_pendingRequest;
    
    // Settings
    int m_selectedRequestType;
    char m_customSystemPrompt[1024];
    bool m_useConversationMode;
    
    // UI Drawing methods
    void DrawChatArea();
    void DrawInputArea();
    void DrawSettingsPanel();
    void DrawRequestTypeSelector();
    void DrawModelSettings();
    
    // Message handling
    void SendMessage();
    void ProcessPendingResponse();
    void HandleAIResponse(const AIResponse& response);
    
    // Utility methods
    std::string GetCurrentTimestamp();
    ImVec4 GetMessageColor(ChatMessage::Type type);
    std::string GetRequestTypeSystemPrompt();
    
    // Predefined request types
    enum RequestType {
        GENERAL_CHAT = 0,
        FEATURE_SUGGESTION,
        CODE_IMPLEMENTATION,
        GAME_LOGIC_ADVICE,
        CODE_OPTIMIZATION,
        CODE_EXPLANATION,
        CUSTOM
    };
    
    const char* GetRequestTypeName(RequestType type);
};

#endif // AIWINDOW_H