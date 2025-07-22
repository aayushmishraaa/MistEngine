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
        
        // Parse error response body for more details
        std::string errorDetails = "";
        if (!httpResponse.body.empty()) {
            // Try to parse the error response - simplified for our basic JSON parser
            try {
                // For now, just extract error message from the response body if it contains "message"
                size_t messagePos = httpResponse.body.find("\"message\":");
                if (messagePos != std::string::npos) {
                    size_t startQuote = httpResponse.body.find("\"", messagePos + 10);
                    if (startQuote != std::string::npos) {
                        size_t endQuote = httpResponse.body.find("\"", startQuote + 1);
                        if (endQuote != std::string::npos) {
                            errorDetails = httpResponse.body.substr(startQuote + 1, endQuote - startQuote - 1);
                        }
                    }
                }
                
                // Also check for type and code fields
                size_t typePos = httpResponse.body.find("\"type\":");
                if (typePos != std::string::npos) {
                    size_t startQuote = httpResponse.body.find("\"", typePos + 7);
                    if (startQuote != std::string::npos) {
                        size_t endQuote = httpResponse.body.find("\"", startQuote + 1);
                        if (endQuote != std::string::npos) {
                            std::string typeValue = httpResponse.body.substr(startQuote + 1, endQuote - startQuote - 1);
                            if (!typeValue.empty()) {
                                errorDetails += " (Type: " + typeValue + ")";
                            }
                        }
                    }
                }
                
                // If we couldn't extract specific fields, use a portion of the raw response
                if (errorDetails.empty()) {
                    errorDetails = httpResponse.body.substr(0, 300);
                }
            } catch (...) {
                // If anything fails, just use raw body
                errorDetails = httpResponse.body.substr(0, 300);
            }
        }
        
        // Provide specific error messages based on status code
        switch (httpResponse.statusCode) {
            case 401:
                response.errorMessage += " (Unauthorized - Invalid API key)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Check your API key is correct (starts with 'sk-')\n";
                response.errorMessage += "2. Ensure your API key is active at https://platform.openai.com/api-keys\n";
                response.errorMessage += "3. Try generating a new API key if the current one is old";
                break;
            case 403:
                response.errorMessage += " (Forbidden - API key lacks permissions)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Check if your API key has the required permissions\n";
                response.errorMessage += "2. Verify your OpenAI account is in good standing\n";
                response.errorMessage += "3. Contact OpenAI support if the issue persists";
                break;
            case 429:
                response.errorMessage += " (Rate limit or quota exceeded)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                if (errorDetails.find("quota") != std::string::npos || 
                    errorDetails.find("billing") != std::string::npos) {
                    response.errorMessage += "\n\n?? QUOTA/BILLING ISSUE:\n";
                    response.errorMessage += "1. Check your billing status at https://platform.openai.com/account/billing\n";
                    response.errorMessage += "2. Add a payment method if you haven't already\n";
                    response.errorMessage += "3. Check if you've exceeded your usage limits\n";
                    response.errorMessage += "4. For new accounts, you may need to add credit first\n";
                    response.errorMessage += "5. Free tier has limited usage - consider upgrading";
                } else {
                    response.errorMessage += "\n\n?? RATE LIMIT SOLUTIONS:\n";
                    response.errorMessage += "1. Wait a moment and try again\n";
                    response.errorMessage += "2. Reduce the frequency of requests\n";
                    response.errorMessage += "3. Consider upgrading your plan for higher limits";
                }
                break;
            case 500:
                response.errorMessage += " (Server error - OpenAI service issue)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Wait a few minutes and try again\n";
                response.errorMessage += "2. Check OpenAI status at https://status.openai.com/\n";
                response.errorMessage += "3. The issue is on OpenAI's side, not yours";
                break;
            case 503:
                response.errorMessage += " (Service unavailable - OpenAI servers overloaded)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Wait and retry in a few minutes\n";
                response.errorMessage += "2. Check OpenAI status page\n";
                response.errorMessage += "3. Try again during off-peak hours";
                break;
            default:
                response.errorMessage += " (Unexpected error)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                } else if (!httpResponse.body.empty()) {
                    response.errorMessage += "\nResponse: " + httpResponse.body.substr(0, 200);
                }
                break;
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