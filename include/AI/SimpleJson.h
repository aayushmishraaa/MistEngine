#ifndef SIMPLEJSON_H
#define SIMPLEJSON_H

#include <string>
#include <map>
#include <vector>

// Simple JSON implementation for basic AI requests
// This is a minimal implementation to avoid external dependencies
// Replace with nlohmann/json when available

class SimpleJson {
public:
    enum Type {
        OBJECT,
        ARRAY,
        STRING,
        NUMBER,
        BOOLEAN,
        NULL_VALUE
    };
    
    SimpleJson() : m_type(NULL_VALUE) {}
    SimpleJson(const std::string& value) : m_type(STRING), m_stringValue(value) {}
    SimpleJson(double value) : m_type(NUMBER), m_numberValue(value) {}
    SimpleJson(bool value) : m_type(BOOLEAN), m_boolValue(value) {}
    
    // Object operations
    SimpleJson& operator[](const std::string& key);
    const SimpleJson& operator[](const std::string& key) const;
    void SetObject();
    bool Contains(const std::string& key) const;
    
    // Array operations
    void SetArray();
    void PushBack(const SimpleJson& value);
    size_t Size() const;
    SimpleJson& operator[](size_t index);
    const SimpleJson& operator[](size_t index) const;
    
    // Value operations
    std::string AsString() const;
    double AsNumber() const;
    bool AsBool() const;
    
    // Serialization
    std::string Dump(int indent = 0) const;
    static SimpleJson Parse(const std::string& jsonStr);
    
    Type GetType() const { return m_type; }
    bool IsString() const { return m_type == STRING; }
    bool IsNumber() const { return m_type == NUMBER; }
    bool IsBool() const { return m_type == BOOLEAN; }
    bool IsObject() const { return m_type == OBJECT; }
    bool IsArray() const { return m_type == ARRAY; }
    bool IsNull() const { return m_type == NULL_VALUE; }
    
private:
    Type m_type;
    std::string m_stringValue;
    double m_numberValue = 0.0;
    bool m_boolValue = false;
    std::map<std::string, SimpleJson> m_objectValue;
    std::vector<SimpleJson> m_arrayValue;
    
    std::string EscapeString(const std::string& str) const;
    std::string UnescapeString(const std::string& str) const;
};

#endif // SIMPLEJSON_H