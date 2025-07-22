#include "AI/SimpleJson.h"
#include <sstream>
#include <iomanip>

SimpleJson& SimpleJson::operator[](const std::string& key) {
    if (m_type != OBJECT) {
        SetObject();
    }
    return m_objectValue[key];
}

const SimpleJson& SimpleJson::operator[](const std::string& key) const {
    static SimpleJson nullJson;
    if (m_type != OBJECT) {
        return nullJson;
    }
    auto it = m_objectValue.find(key);
    return (it != m_objectValue.end()) ? it->second : nullJson;
}

void SimpleJson::SetObject() {
    m_type = OBJECT;
    m_objectValue.clear();
}

bool SimpleJson::Contains(const std::string& key) const {
    return m_type == OBJECT && m_objectValue.find(key) != m_objectValue.end();
}

void SimpleJson::SetArray() {
    m_type = ARRAY;
    m_arrayValue.clear();
}

void SimpleJson::PushBack(const SimpleJson& value) {
    if (m_type != ARRAY) {
        SetArray();
    }
    m_arrayValue.push_back(value);
}

size_t SimpleJson::Size() const {
    if (m_type == ARRAY) {
        return m_arrayValue.size();
    } else if (m_type == OBJECT) {
        return m_objectValue.size();
    }
    return 0;
}

SimpleJson& SimpleJson::operator[](size_t index) {
    static SimpleJson nullJson;
    if (m_type != ARRAY || index >= m_arrayValue.size()) {
        return nullJson;
    }
    return m_arrayValue[index];
}

const SimpleJson& SimpleJson::operator[](size_t index) const {
    static SimpleJson nullJson;
    if (m_type != ARRAY || index >= m_arrayValue.size()) {
        return nullJson;
    }
    return m_arrayValue[index];
}

std::string SimpleJson::AsString() const {
    if (m_type == STRING) {
        return m_stringValue;
    } else if (m_type == NUMBER) {
        return std::to_string(m_numberValue);
    } else if (m_type == BOOLEAN) {
        return m_boolValue ? "true" : "false";
    }
    return "";
}

double SimpleJson::AsNumber() const {
    if (m_type == NUMBER) {
        return m_numberValue;
    } else if (m_type == STRING) {
        try {
            return std::stod(m_stringValue);
        } catch (...) {
            return 0.0;
        }
    }
    return 0.0;
}

bool SimpleJson::AsBool() const {
    if (m_type == BOOLEAN) {
        return m_boolValue;
    } else if (m_type == STRING) {
        return m_stringValue == "true";
    } else if (m_type == NUMBER) {
        return m_numberValue != 0.0;
    }
    return false;
}

std::string SimpleJson::Dump(int indent) const {
    std::ostringstream oss;
    std::string indentStr(indent, ' ');
    
    switch (m_type) {
        case STRING:
            oss << "\"" << EscapeString(m_stringValue) << "\"";
            break;
        case NUMBER:
            oss << m_numberValue;
            break;
        case BOOLEAN:
            oss << (m_boolValue ? "true" : "false");
            break;
        case NULL_VALUE:
            oss << "null";
            break;
        case OBJECT:
            oss << "{\n";
            {
                bool first = true;
                for (const auto& pair : m_objectValue) {
                    if (!first) oss << ",\n";
                    oss << std::string(indent + 2, ' ') << "\"" << EscapeString(pair.first) << "\": ";
                    oss << pair.second.Dump(indent + 2);
                    first = false;
                }
            }
            oss << "\n" << indentStr << "}";
            break;
        case ARRAY:
            oss << "[\n";
            for (size_t i = 0; i < m_arrayValue.size(); ++i) {
                if (i > 0) oss << ",\n";
                oss << std::string(indent + 2, ' ') << m_arrayValue[i].Dump(indent + 2);
            }
            oss << "\n" << indentStr << "]";
            break;
    }
    
    return oss.str();
}

SimpleJson SimpleJson::Parse(const std::string& jsonStr) {
    // Very basic JSON parsing - implement as needed
    // For now, return empty object
    SimpleJson result;
    result.SetObject();
    return result;
}

std::string SimpleJson::EscapeString(const std::string& str) const {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string SimpleJson::UnescapeString(const std::string& str) const {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"': result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}