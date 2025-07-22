#include "AI/OpenAIProvider.h"
#include "AI/SimpleJson.h"
#include <future>
#include <thread>
#include <iostream>

OpenAIProvider::OpenAIProvider() 
    : m_initialized(false)
    , m_endpoint("https://api.openai.com/v1/chat/completions")
    , m_apiVersion("2023-05-15")
    , m_httpClient(std::make_unique<HttpClient>()) {
}

bool OpenAIProvider::Initialize(const std::string& apiKey, const std::string& endpoint) {
    if (apiKey.empty()) {
        std::cerr << "OpenAI API key cannot be empty" << std::endl;
        return false;
    }
    
    m_apiKey = apiKey;
    if (!endpoint.empty()) {
        m_endpoint = endpoint;
    }
    
    m_httpClient->SetUserAgent("MistEngine/1.0 OpenAI-Client");
    m_httpClient->SetTimeout(30);
    
    m_initialized = true;
    return true;
}

bool OpenAIProvider::IsInitialized() const {
    return m_initialized;
}

std::string OpenAIProvider::GetProviderName() const {
    return "OpenAI";
}

AIResponse OpenAIProvider::SendRequest(const AIRequest& request) {
    if (!m_initialized) {
        return AIResponse(false, "Provider not initialized");
    }
    
    std::string payload = BuildRequestPayload(request);
    auto headers = GetDefaultHeaders();
    
    HttpResponse httpResponse = m_httpClient->Post(m_endpoint, payload, headers);
    return ParseResponse(httpResponse);
}

std::future<AIResponse> OpenAIProvider::SendRequestAsync(const AIRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return SendRequest(request);
    });
}

std::vector<std::string> OpenAIProvider::GetAvailableModels() const {
    return {
        "gpt-4",
        "gpt-4-turbo-preview",
        "gpt-3.5-turbo",
        "gpt-3.5-turbo-16k"
    };
}

void OpenAIProvider::SetOrganization(const std::string& organization) {
    m_organization = organization;
}

void OpenAIProvider::SetApiVersion(const std::string& version) {
    m_apiVersion = version;
}

std::string OpenAIProvider::BuildRequestPayload(const AIRequest& request) {
    SimpleJson payload;
    payload.SetObject();
    
    payload["model"] = SimpleJson(request.model);
    payload["temperature"] = SimpleJson(request.temperature);
    payload["max_tokens"] = SimpleJson(static_cast<double>(request.maxTokens));
    
    SimpleJson messages;
    messages.SetArray();
    
    // Add system message if provided
    if (!request.systemPrompt.empty()) {
        SimpleJson systemMsg;
        systemMsg.SetObject();
        systemMsg["role"] = SimpleJson("system");
        systemMsg["content"] = SimpleJson(request.systemPrompt);
        messages.PushBack(systemMsg);
    }
    
    // Add conversation messages
    for (const auto& msg : request.messages) {
        SimpleJson jsonMsg;
        jsonMsg.SetObject();
        
        std::string role;
        switch (msg.role) {
            case AIMessage::USER: role = "user"; break;
            case AIMessage::ASSISTANT: role = "assistant"; break;
            case AIMessage::SYSTEM: role = "system"; break;
        }
        
        jsonMsg["role"] = SimpleJson(role);
        jsonMsg["content"] = SimpleJson(msg.content);
        messages.PushBack(jsonMsg);
    }
    
    payload["messages"] = messages;
    
    return payload.Dump();
}

AIResponse OpenAIProvider::ParseResponse(const HttpResponse& httpResponse) {
    AIResponse response;
    
    if (!httpResponse.success) {
        response.success = false;
        response.errorMessage = "HTTP request failed: " + httpResponse.errorMessage;
        return response;
    }
    
    if (httpResponse.statusCode != 200) {
        response.success = false;
        response.errorMessage = "API request failed with status: " + std::to_string(httpResponse.statusCode);
        
        // Provide more specific error messages based on status code
        switch (httpResponse.statusCode) {
            case 401:
                response.errorMessage += " (Unauthorized - check your API key)";
                break;
            case 403:
                response.errorMessage += " (Forbidden - API key may not have required permissions)";
                break;
            case 429:
                response.errorMessage += " (Rate limit exceeded - too many requests)";
                break;
            case 500:
                response.errorMessage += " (Server error - try again later)";
                break;
            case 503:
                response.errorMessage += " (Service unavailable - OpenAI servers may be down)";
                break;
            default:
                response.errorMessage += " (Unexpected error)";
                break;
        }
        
        if (!httpResponse.body.empty()) {
            response.errorMessage += "\nResponse: " + httpResponse.body.substr(0, 200);
        }
        return response;
    }
    
    if (httpResponse.body.empty()) {
        response.success = false;
        response.errorMessage = "Empty response from API";
        return response;
    }
    
    // Basic JSON parsing since we don't have nlohmann/json yet
    // Look for the content field in the response
    std::string body = httpResponse.body;
    
    // Find "content": "..." pattern
    size_t contentStart = body.find("\"content\":");
    if (contentStart == std::string::npos) {
        // If we can't find content, maybe there's an error message
        size_t errorStart = body.find("\"error\":");
        if (errorStart != std::string::npos) {
            size_t messageStart = body.find("\"message\":", errorStart);
            if (messageStart != std::string::npos) {
                messageStart = body.find("\"", messageStart + 10);
                if (messageStart != std::string::npos) {
                    size_t messageEnd = body.find("\"", messageStart + 1);
                    if (messageEnd != std::string::npos) {
                        response.success = false;
                        response.errorMessage = "API Error: " + body.substr(messageStart + 1, messageEnd - messageStart - 1);
                        return response;
                    }
                }
            }
        }
        
        response.success = false;
        response.errorMessage = "Could not parse response - no content field found.\nResponse preview: " + body.substr(0, 300);
        return response;
    }
    
    // Find the start of the content string
    contentStart = body.find("\"", contentStart + 10);
    if (contentStart == std::string::npos) {
        response.success = false;
        response.errorMessage = "Invalid response format";
        return response;
    }
    
    // Find the end of the content string (this is simplified and may not handle escaped quotes properly)
    size_t contentEnd = contentStart + 1;
    while (contentEnd < body.length()) {
        if (body[contentEnd] == '"' && body[contentEnd - 1] != '\\') {
            break;
        }
        contentEnd++;
    }
    
    if (contentEnd >= body.length()) {
        response.success = false;
        response.errorMessage = "Could not find end of content in response";
        return response;
    }
    
    response.success = true;
    response.content = body.substr(contentStart + 1, contentEnd - contentStart - 1);
    
    // Simple unescape for common characters
    std::string& content = response.content;
    size_t pos = 0;
    while ((pos = content.find("\\n", pos)) != std::string::npos) {
        content.replace(pos, 2, "\n");
        pos += 1;
    }
    pos = 0;
    while ((pos = content.find("\\\"", pos)) != std::string::npos) {
        content.replace(pos, 2, "\"");
        pos += 1;
    }
    pos = 0;
    while ((pos = content.find("\\\\", pos)) != std::string::npos) {
        content.replace(pos, 2, "\\");
        pos += 1;
    }
    
    return response;
}

std::string OpenAIProvider::GetAuthHeader() const {
    return "Bearer " + m_apiKey;
}

std::map<std::string, std::string> OpenAIProvider::GetDefaultHeaders() const {
    std::map<std::string, std::string> headers;
    headers["Authorization"] = GetAuthHeader();
    headers["Content-Type"] = "application/json";
    
    if (!m_organization.empty()) {
        headers["OpenAI-Organization"] = m_organization;
    }
    
    // For Azure OpenAI
    if (m_endpoint.find("openai.azure.com") != std::string::npos) {
        headers["api-key"] = m_apiKey; // Azure uses api-key instead of Authorization
        headers.erase("Authorization");
    }
    
    return headers;
}