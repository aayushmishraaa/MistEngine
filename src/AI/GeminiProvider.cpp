#include "AI/GeminiProvider.h"
#include "AI/SimpleJson.h"
#include <future>
#include <thread>
#include <iostream>
#include <sstream>

GeminiProvider::GeminiProvider() 
    : m_initialized(false)
    , m_endpoint("https://generativelanguage.googleapis.com/v1/models/gemini-1.5-flash:generateContent")
    , m_apiVersion("v1")
    , m_httpClient(std::make_unique<HttpClient>()) {
}

bool GeminiProvider::Initialize(const std::string& apiKey, const std::string& endpoint) {
    if (apiKey.empty()) {
        std::cerr << "Gemini API key cannot be empty" << std::endl;
        return false;
    }
    
    m_apiKey = apiKey;
    if (!endpoint.empty()) {
        m_endpoint = endpoint;
    } else {
        // Set default endpoint with API key using the stable v1 API
        m_endpoint = "https://generativelanguage.googleapis.com/v1/models/gemini-1.5-flash:generateContent?key=" + m_apiKey;
    }
    
    m_httpClient->SetUserAgent("MistEngine/1.0 Gemini-Client");
    m_httpClient->SetTimeout(30);
    
    m_initialized = true;
    return true;
}

bool GeminiProvider::IsInitialized() const {
    return m_initialized;
}

std::string GeminiProvider::GetProviderName() const {
    return "Google Gemini";
}

AIResponse GeminiProvider::SendRequest(const AIRequest& request) {
    if (!m_initialized) {
        return AIResponse(false, "Provider not initialized");
    }
    
    std::string payload = BuildRequestPayload(request);
    auto headers = GetDefaultHeaders();
    
    HttpResponse httpResponse = m_httpClient->Post(m_endpoint, payload, headers);
    return ParseResponse(httpResponse);
}

std::future<AIResponse> GeminiProvider::SendRequestAsync(const AIRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return SendRequest(request);
    });
}

std::vector<std::string> GeminiProvider::GetAvailableModels() const {
    return {
        "gemini-1.5-flash",
        "gemini-1.5-pro",
        "gemini-1.0-pro"
    };
}

void GeminiProvider::SetApiVersion(const std::string& version) {
    m_apiVersion = version;
}

std::string GeminiProvider::BuildRequestPayload(const AIRequest& request) {
    SimpleJson payload;
    payload.SetObject();
    
    // Gemini uses a different structure than OpenAI
    SimpleJson contents;
    contents.SetArray();
    
    // Combine system prompt and user messages
    std::string combinedContent = "";
    if (!request.systemPrompt.empty()) {
        combinedContent += request.systemPrompt + "\n\n";
    }
    
    // Add all messages
    for (const auto& msg : request.messages) {
        switch (msg.role) {
            case AIMessage::USER:
                combinedContent += "User: " + msg.content + "\n";
                break;
            case AIMessage::ASSISTANT:
                combinedContent += "Assistant: " + msg.content + "\n";
                break;
            case AIMessage::SYSTEM:
                combinedContent += msg.content + "\n";
                break;
        }
    }
    
    // Create content part
    SimpleJson contentPart;
    contentPart.SetObject();
    
    SimpleJson parts;
    parts.SetArray();
    
    SimpleJson textPart;
    textPart.SetObject();
    textPart["text"] = SimpleJson(combinedContent);
    parts.PushBack(textPart);
    
    contentPart["parts"] = parts;
    contents.PushBack(contentPart);
    
    payload["contents"] = contents;
    
    // Generation config
    SimpleJson generationConfig;
    generationConfig.SetObject();
    generationConfig["temperature"] = SimpleJson(request.temperature);
    generationConfig["maxOutputTokens"] = SimpleJson(static_cast<double>(request.maxTokens));
    generationConfig["topP"] = SimpleJson(0.8);
    generationConfig["topK"] = SimpleJson(10.0);
    
    payload["generationConfig"] = generationConfig;
    
    return payload.Dump();
}

AIResponse GeminiProvider::ParseResponse(const HttpResponse& httpResponse) {
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
            // Try to extract error message from Gemini response
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
            
            if (errorDetails.empty()) {
                errorDetails = httpResponse.body.substr(0, 300);
            }
        }
        
        // Provide specific error messages based on status code
        switch (httpResponse.statusCode) {
            case 400:
                response.errorMessage += " (Bad Request - Check your request format)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Check your request parameters\n";
                response.errorMessage += "2. Ensure your API key is correct\n";
                response.errorMessage += "3. Verify the model name is supported";
                break;
            case 401:
                response.errorMessage += " (Unauthorized - Invalid API key)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Get API key from https://aistudio.google.com/app/apikey\n";
                response.errorMessage += "2. Ensure API key is active and not expired\n";
                response.errorMessage += "3. Check that Gemini API is enabled for your project";
                break;
            case 403:
                response.errorMessage += " (Forbidden - Access denied)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Enable the Gemini API in Google Cloud Console\n";
                response.errorMessage += "2. Check your quota and billing settings\n";
                response.errorMessage += "3. Verify your account has access to Gemini";
                break;
            case 404:
                response.errorMessage += " (Not Found - Model or endpoint not available)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Update to use gemini-1.5-flash model\n";
                response.errorMessage += "2. Use stable v1 API instead of v1beta\n";
                response.errorMessage += "3. Older model names (gemini-pro) are deprecated\n";
                response.errorMessage += "4. Try restarting the application with updated settings";
                break;
            case 429:
                response.errorMessage += " (Rate limit exceeded)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Wait a moment and try again\n";
                response.errorMessage += "2. Reduce request frequency\n";
                response.errorMessage += "3. Check your quota limits";
                break;
            case 500:
            case 503:
                response.errorMessage += " (Server error - Google service issue)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                response.errorMessage += "\n\n?? SOLUTIONS:\n";
                response.errorMessage += "1. Wait a few minutes and try again\n";
                response.errorMessage += "2. Check Google Cloud status\n";
                response.errorMessage += "3. The issue is on Google's side";
                break;
            default:
                response.errorMessage += " (Unexpected error)";
                if (!errorDetails.empty()) {
                    response.errorMessage += "\nDetails: " + errorDetails;
                }
                break;
        }
        
        return response;
    }
    
    if (httpResponse.body.empty()) {
        response.success = false;
        response.errorMessage = "Empty response body";
        return response;
    }
    
    // Parse Gemini response format
    try {
        // Look for the text content in Gemini's response structure
        // Gemini response: {"candidates":[{"content":{"parts":[{"text":"response"}]}}]}
        
        size_t textPos = httpResponse.body.find("\"text\":");
        if (textPos != std::string::npos) {
            size_t startQuote = httpResponse.body.find("\"", textPos + 7);
            if (startQuote != std::string::npos) {
                size_t endQuote = startQuote + 1;
                int escapeCount = 0;
                
                // Find the actual end quote, handling escaped quotes
                while (endQuote < httpResponse.body.length()) {
                    if (httpResponse.body[endQuote] == '\\') {
                        escapeCount++;
                    } else if (httpResponse.body[endQuote] == '"' && escapeCount % 2 == 0) {
                        break;
                    } else {
                        escapeCount = 0;
                    }
                    endQuote++;
                }
                
                if (endQuote < httpResponse.body.length()) {
                    response.content = httpResponse.body.substr(startQuote + 1, endQuote - startQuote - 1);
                    
                    // Basic unescape for common characters
                    size_t pos = 0;
                    while ((pos = response.content.find("\\n", pos)) != std::string::npos) {
                        response.content.replace(pos, 2, "\n");
                        pos += 1;
                    }
                    pos = 0;
                    while ((pos = response.content.find("\\\"", pos)) != std::string::npos) {
                        response.content.replace(pos, 2, "\"");
                        pos += 1;
                    }
                    
                    response.success = true;
                    response.errorMessage = "";
                    return response;
                }
            }
        }
        
        response.success = false;
        response.errorMessage = "Could not parse response content from: " + httpResponse.body.substr(0, 200);
        
    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = "Failed to parse JSON response: " + std::string(e.what());
    }
    
    return response;
}

std::map<std::string, std::string> GeminiProvider::GetDefaultHeaders() {
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    // Gemini uses API key in URL parameter, not in headers
    return headers;
}