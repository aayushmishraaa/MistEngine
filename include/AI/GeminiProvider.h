#ifndef GEMINIPROVIDER_H
#define GEMINIPROVIDER_H

#include "AI/AIProvider.h"
#include "AI/HttpClient.h"
#include <memory>

class GeminiProvider : public AIProvider {
public:
    GeminiProvider();
    ~GeminiProvider() override = default;
    
    // AIProvider interface
    bool Initialize(const std::string& apiKey, const std::string& endpoint = "") override;
    bool IsInitialized() const override;
    std::string GetProviderName() const override;
    
    AIResponse SendRequest(const AIRequest& request) override;
    std::future<AIResponse> SendRequestAsync(const AIRequest& request) override;
    
    std::vector<std::string> GetAvailableModels() const override;
    
    // Gemini-specific methods
    void SetApiVersion(const std::string& version);
    
private:
    bool m_initialized;
    std::string m_apiKey;
    std::string m_endpoint;
    std::string m_apiVersion;
    std::unique_ptr<HttpClient> m_httpClient;
    
    // Helper methods
    std::string BuildRequestPayload(const AIRequest& request);
    AIResponse ParseResponse(const HttpResponse& httpResponse);
    std::map<std::string, std::string> GetDefaultHeaders();
};

#endif // GEMINIPROVIDER_H