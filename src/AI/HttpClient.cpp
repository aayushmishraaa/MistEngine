#include "AI/HttpClient.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

// libcurl write callback
static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::string* response = static_cast<std::string*>(userdata);
    size_t totalSize = size * nmemb;
    response->append(ptr, totalSize);
    return totalSize;
}

class HttpClient::Impl {
public:
    int timeout = 30;
    std::string userAgent = "MistEngine/1.0";

    HttpResponse SendRequest(const std::string& method, const std::string& url,
                             const std::string& body, const std::map<std::string, std::string>& headers) {
        HttpResponse response;

        CURL* curl = curl_easy_init();
        if (!curl) {
            response.success = false;
            response.errorMessage = "Failed to initialize libcurl";
            return response;
        }

        std::string responseBody;

        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set method
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        } else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        } else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        // GET is the default

        // Set headers
        struct curl_slist* headerList = nullptr;
        for (const auto& header : headers) {
            std::string headerStr = header.first + ": " + header.second;
            headerList = curl_slist_append(headerList, headerStr.c_str());
        }
        if (headerList) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        }

        // Set user agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());

        // Set timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout));
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(timeout));

        // Set write callback
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Log request
        std::cout << "HTTP Request: " << method << " " << url << std::endl;
        if (!body.empty()) {
            std::cout << "Body length: " << body.length() << " characters" << std::endl;
        }

        // Perform request
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            response.success = false;
            response.errorMessage = "HTTP request failed: " + std::string(curl_easy_strerror(res));

            switch (res) {
                case CURLE_OPERATION_TIMEDOUT:
                    response.errorMessage += " (Timeout - check internet connection)";
                    break;
                case CURLE_COULDNT_RESOLVE_HOST:
                    response.errorMessage += " (DNS resolution failed)";
                    break;
                case CURLE_COULDNT_CONNECT:
                    response.errorMessage += " (Cannot connect to server)";
                    break;
                case CURLE_SSL_CONNECT_ERROR:
                    response.errorMessage += " (SSL connection error)";
                    break;
                default:
                    break;
            }

            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);
            return response;
        }

        // Get status code
        long statusCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        response.statusCode = static_cast<int>(statusCode);
        response.body = responseBody;
        response.success = (response.statusCode >= 200 && response.statusCode < 300);

        // Log response
        std::cout << "HTTP Response: " << response.statusCode << std::endl;
        std::cout << "Response body length: " << responseBody.length() << " characters" << std::endl;

        if (!response.success) {
            std::cout << "Response body: " << responseBody.substr(0, 500) << "..." << std::endl;
        }

        // Cleanup
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);

        return response;
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
