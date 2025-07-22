#include "AI/HttpClient.h"
#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "wininet.lib")

// Simple implementation using Windows WinINet API
// For now, this is a basic stub that you can replace with actual HTTP implementation

class HttpClient::Impl {
public:
    int timeout = 30;
    std::string userAgent = "MistEngine/1.0";
    
    HttpResponse SendRequest(const std::string& method, const std::string& url, 
                           const std::string& body, const std::map<std::string, std::string>& headers) {
        HttpResponse response;
        
        // Parse URL
        std::string hostname, path;
        int port = 443; // Default to HTTPS
        bool isHttps = true;
        
        if (!ParseUrl(url, hostname, path, port, isHttps)) {
            response.success = false;
            response.errorMessage = "Invalid URL format: " + url;
            return response;
        }
        
        // Initialize WinINet
        HINTERNET hInternet = InternetOpenA(userAgent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            DWORD error = GetLastError();
            response.success = false;
            response.errorMessage = "Failed to initialize WinINet. Error code: " + std::to_string(error);
            return response;
        }
        
        // Set timeout
        DWORD timeoutMs = timeout * 1000;
        InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
        InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
        InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeoutMs, sizeof(timeoutMs));
        
        // Connect to server
        HINTERNET hConnect = InternetConnectA(hInternet, hostname.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) {
            DWORD error = GetLastError();
            InternetCloseHandle(hInternet);
            response.success = false;
            response.errorMessage = "Failed to connect to server: " + hostname + ":" + std::to_string(port) + ". Error code: " + std::to_string(error);
            return response;
        }
        
        // Create request
        DWORD requestFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE;
        if (isHttps) {
            requestFlags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
        }
        
        HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, NULL, requestFlags, 0);
        if (!hRequest) {
            DWORD error = GetLastError();
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            response.success = false;
            response.errorMessage = "Failed to create HTTP request. Error code: " + std::to_string(error);
            return response;
        }
        
        // Build headers string
        std::string headersStr;
        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
            headersStr += it->first + ": " + it->second + "\r\n";
        }
        
        // Log the request for debugging
        std::cout << "HTTP Request: " << method << " " << url << std::endl;
        std::cout << "Headers: " << headersStr << std::endl;
        if (!body.empty()) {
            std::cout << "Body length: " << body.length() << " characters" << std::endl;
        }
        
        // Send request
        BOOL result = HttpSendRequestA(hRequest, 
                                      headersStr.empty() ? NULL : headersStr.c_str(), 
                                      static_cast<DWORD>(headersStr.length()),
                                      body.empty() ? NULL : (LPVOID)body.c_str(),
                                      static_cast<DWORD>(body.length()));
        
        if (!result) {
            DWORD error = GetLastError();
            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            response.success = false;
            response.errorMessage = "Failed to send HTTP request. Error code: " + std::to_string(error);
            
            // Provide more specific error messages
            switch (error) {
                case ERROR_INTERNET_TIMEOUT:
                    response.errorMessage += " (Timeout - check internet connection)";
                    break;
                case ERROR_INTERNET_NAME_NOT_RESOLVED:
                    response.errorMessage += " (DNS resolution failed)";
                    break;
                case ERROR_INTERNET_CANNOT_CONNECT:
                    response.errorMessage += " (Cannot connect to server)";
                    break;
                case ERROR_INTERNET_CONNECTION_ABORTED:
                    response.errorMessage += " (Connection aborted)";
                    break;
                case ERROR_INTERNET_CONNECTION_RESET:
                    response.errorMessage += " (Connection reset)";
                    break;
                default:
                    response.errorMessage += " (Unknown network error)";
                    break;
            }
            
            return response;
        }
        
        // Get status code
        DWORD statusCode;
        DWORD statusCodeSize = sizeof(statusCode);
        if (HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL)) {
            response.statusCode = static_cast<int>(statusCode);
        } else {
            response.statusCode = 0;
        }
        
        // Get response headers
        char headerBuffer[8192];
        DWORD headerSize = sizeof(headerBuffer);
        if (HttpQueryInfoA(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, headerBuffer, &headerSize, NULL)) {
            // Parse headers (simplified)
            std::string headerStr(headerBuffer, headerSize);
            // You could parse individual headers here if needed
        }
        
        // Read response
        std::string responseBody;
        char buffer[4096];
        DWORD bytesRead;
        
        while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            responseBody += buffer;
        }
        
        response.body = responseBody;
        response.success = (response.statusCode >= 200 && response.statusCode < 300);
        
        // Log response for debugging
        std::cout << "HTTP Response: " << response.statusCode << std::endl;
        std::cout << "Response body length: " << responseBody.length() << " characters" << std::endl;
        
        if (!response.success) {
            std::cout << "Response body: " << responseBody.substr(0, 500) << "..." << std::endl;
        }
        
        // Cleanup
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        return response;
    }
    
private:
    bool ParseUrl(const std::string& url, std::string& hostname, std::string& path, int& port, bool& isHttps) {
        // Simple URL parsing
        size_t protocolEnd = url.find("://");
        if (protocolEnd == std::string::npos) {
            return false;
        }
        
        std::string protocol = url.substr(0, protocolEnd);
        isHttps = (protocol == "https");
        port = isHttps ? 443 : 80;
        
        size_t hostStart = protocolEnd + 3;
        size_t pathStart = url.find('/', hostStart);
        
        if (pathStart == std::string::npos) {
            hostname = url.substr(hostStart);
            path = "/";
        } else {
            hostname = url.substr(hostStart, pathStart - hostStart);
            path = url.substr(pathStart);
        }
        
        // Check for port in hostname
        size_t portPos = hostname.find(':');
        if (portPos != std::string::npos) {
            try {
                port = std::stoi(hostname.substr(portPos + 1));
                hostname = hostname.substr(0, portPos);
            } catch (...) {
                // Invalid port, use default
            }
        }
        
        return !hostname.empty();
    }
};

HttpClient::HttpClient() : pImpl(std::make_unique<Impl>()) {
}

HttpClient::~HttpClient() = default;

HttpResponse HttpClient::Get(const std::string& url, const std::map<std::string, std::string>& headers) {
    return pImpl->SendRequest("GET", url, "", headers);
}

HttpResponse HttpClient::Post(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers) {
    return pImpl->SendRequest("POST", url, body, headers);
}

HttpResponse HttpClient::Put(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers) {
    return pImpl->SendRequest("PUT", url, body, headers);
}

HttpResponse HttpClient::Delete(const std::string& url, const std::map<std::string, std::string>& headers) {
    return pImpl->SendRequest("DELETE", url, "", headers);
}

void HttpClient::SetTimeout(int timeoutSeconds) {
    pImpl->timeout = timeoutSeconds;
}

void HttpClient::SetUserAgent(const std::string& userAgent) {
    pImpl->userAgent = userAgent;
}