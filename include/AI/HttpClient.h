#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <string>
#include <map>
#include <memory>

struct HttpResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success;
    std::string errorMessage;
    
    HttpResponse() : statusCode(0), success(false) {}
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    // HTTP Methods
    HttpResponse Get(const std::string& url, const std::map<std::string, std::string>& headers = {});
    HttpResponse Post(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {});
    HttpResponse Put(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {});
    HttpResponse Delete(const std::string& url, const std::map<std::string, std::string>& headers = {});
    
    // Configuration
    void SetTimeout(int timeoutSeconds);
    void SetUserAgent(const std::string& userAgent);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // HTTPCLIENT_H