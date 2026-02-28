#include "AI/SimpleJson.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cmath>

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
    if (m_type != ARRAY || index >= m_arrayValue.size()) {
        throw std::out_of_range("SimpleJson array index out of range");
    }
    return m_arrayValue[index];
}

const SimpleJson& SimpleJson::operator[](size_t index) const {
    if (m_type != ARRAY || index >= m_arrayValue.size()) {
        throw std::out_of_range("SimpleJson array index out of range");
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
            if (m_numberValue == std::floor(m_numberValue) &&
                std::abs(m_numberValue) < 1e15) {
                oss << static_cast<long long>(m_numberValue);
            } else {
                oss << m_numberValue;
            }
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

// ---- Recursive-descent JSON parser ----

namespace {

void SkipWhitespace(const std::string& s, size_t& pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r')) {
        ++pos;
    }
}

std::string ParseString(const std::string& s, size_t& pos) {
    if (pos >= s.size() || s[pos] != '"') {
        throw std::runtime_error("Expected '\"' at position " + std::to_string(pos));
    }
    ++pos; // skip opening quote

    std::string result;
    while (pos < s.size()) {
        char c = s[pos];
        if (c == '"') {
            ++pos; // skip closing quote
            return result;
        }
        if (c == '\\') {
            ++pos;
            if (pos >= s.size()) break;
            char esc = s[pos];
            switch (esc) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u': {
                    // Parse 4 hex digits (basic BMP only)
                    if (pos + 4 >= s.size()) {
                        result += '?';
                        break;
                    }
                    std::string hex = s.substr(pos + 1, 4);
                    pos += 4;
                    unsigned long cp = std::stoul(hex, nullptr, 16);
                    if (cp < 0x80) {
                        result += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        result += static_cast<char>(0xC0 | (cp >> 6));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (cp >> 12));
                        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: result += esc; break;
            }
        } else {
            result += c;
        }
        ++pos;
    }
    throw std::runtime_error("Unterminated string");
}

SimpleJson ParseValue(const std::string& s, size_t& pos);

SimpleJson ParseObject(const std::string& s, size_t& pos) {
    SimpleJson obj;
    obj.SetObject();
    ++pos; // skip '{'
    SkipWhitespace(s, pos);

    if (pos < s.size() && s[pos] == '}') {
        ++pos;
        return obj;
    }

    while (pos < s.size()) {
        SkipWhitespace(s, pos);
        std::string key = ParseString(s, pos);
        SkipWhitespace(s, pos);

        if (pos >= s.size() || s[pos] != ':') {
            throw std::runtime_error("Expected ':' at position " + std::to_string(pos));
        }
        ++pos; // skip ':'
        SkipWhitespace(s, pos);

        obj[key] = ParseValue(s, pos);
        SkipWhitespace(s, pos);

        if (pos < s.size() && s[pos] == ',') {
            ++pos;
        } else if (pos < s.size() && s[pos] == '}') {
            ++pos;
            return obj;
        } else {
            throw std::runtime_error("Expected ',' or '}' at position " + std::to_string(pos));
        }
    }
    throw std::runtime_error("Unterminated object");
}

SimpleJson ParseArray(const std::string& s, size_t& pos) {
    SimpleJson arr;
    arr.SetArray();
    ++pos; // skip '['
    SkipWhitespace(s, pos);

    if (pos < s.size() && s[pos] == ']') {
        ++pos;
        return arr;
    }

    while (pos < s.size()) {
        SkipWhitespace(s, pos);
        arr.PushBack(ParseValue(s, pos));
        SkipWhitespace(s, pos);

        if (pos < s.size() && s[pos] == ',') {
            ++pos;
        } else if (pos < s.size() && s[pos] == ']') {
            ++pos;
            return arr;
        } else {
            throw std::runtime_error("Expected ',' or ']' at position " + std::to_string(pos));
        }
    }
    throw std::runtime_error("Unterminated array");
}

SimpleJson ParseNumber(const std::string& s, size_t& pos) {
    size_t start = pos;
    if (pos < s.size() && s[pos] == '-') ++pos;

    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
    if (pos < s.size() && s[pos] == '.') {
        ++pos;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
    }
    if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
        ++pos;
        if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
    }

    std::string numStr = s.substr(start, pos - start);
    return SimpleJson(std::stod(numStr));
}

SimpleJson ParseValue(const std::string& s, size_t& pos) {
    SkipWhitespace(s, pos);
    if (pos >= s.size()) {
        throw std::runtime_error("Unexpected end of input");
    }

    char c = s[pos];

    if (c == '"') {
        return SimpleJson(ParseString(s, pos));
    }
    if (c == '{') {
        return ParseObject(s, pos);
    }
    if (c == '[') {
        return ParseArray(s, pos);
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        return ParseNumber(s, pos);
    }
    if (s.compare(pos, 4, "true") == 0) {
        pos += 4;
        return SimpleJson(true);
    }
    if (s.compare(pos, 5, "false") == 0) {
        pos += 5;
        return SimpleJson(false);
    }
    if (s.compare(pos, 4, "null") == 0) {
        pos += 4;
        return SimpleJson();
    }

    throw std::runtime_error("Unexpected character '" + std::string(1, c) + "' at position " + std::to_string(pos));
}

} // anonymous namespace

SimpleJson SimpleJson::Parse(const std::string& jsonStr) {
    size_t pos = 0;
    SimpleJson result = ParseValue(jsonStr, pos);
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
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
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
