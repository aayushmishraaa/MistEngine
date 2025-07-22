#ifndef OPENAIPROVIDER_H
#define OPENAIPROVIDER_H

#include "AIProvider.h"
#include "HttpClient.h"
#include <string>
#include <memory>

class OpenAIProvider : public AIProvider {
public:
    OpenAIProvider();
    virtual ~OpenAIProvider() = default;
    
    // AIProvider implementation
    bool Initialize(const std::string& apiKey, const std::string& endpoint = "") override;
    bool IsInitialized() const override;
    std::string GetProviderName() const override;
    
    AIResponse SendRequest(const AIRequest& request) override;
    std::future<AIResponse> SendRequestAsync(const AIRequest& request) override;
    
    std::vector<std::string> GetAvailableModels() const override;
    
    // OpenAI specific configuration
    void SetOrganization(const std::string& organization);
    void SetApiVersion(const std::string& version); // For Azure OpenAI
    
private:
    std::string m_apiKey;
    std::string m_endpoint;
    std::string m_organization;
    std::string m_apiVersion;
    bool m_initialized;
    std::unique_ptr<HttpClient> m_httpClient;
    
    // Helper methods
    std::string BuildRequestPayload(const AIRequest& request);
    AIResponse ParseResponse(const HttpResponse& httpResponse);
    std::string GetAuthHeader() const;
    std::map<std::string, std::string> GetDefaultHeaders() const;
};

#endif // OPENAIPROVIDER_H