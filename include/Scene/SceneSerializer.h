#pragma once
#ifndef MIST_SCENE_SERIALIZER_H
#define MIST_SCENE_SERIALIZER_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

class Coordinator;
class Scene;
struct TransformComponent;
struct RenderComponent;
struct PhysicsComponent;

typedef uint32_t Entity;

// Minimal JSON writer (no external dependency)
class JsonWriter {
public:
    void BeginObject();
    void EndObject();
    void BeginArray(const std::string& key);
    void EndArray();
    void Key(const std::string& key);
    void String(const std::string& value);
    void Number(float value);
    void Number(int value);
    void Bool(bool value);
    void Vec3(const std::string& key, const glm::vec3& v);
    std::string GetString() const { return m_Stream.str(); }

private:
    std::ostringstream m_Stream;
    bool m_NeedComma = false;
    int m_Indent = 0;

    void writeIndent();
    void writeComma();
};

// Minimal JSON reader
struct JsonValue {
    enum Type { NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT };
    Type type = NUL;
    std::string strValue;
    float numValue = 0.0f;
    bool boolValue = false;
    std::vector<JsonValue> arrayValues;
    std::vector<std::pair<std::string, JsonValue>> objectValues;

    const JsonValue& operator[](const std::string& key) const;
    const JsonValue& operator[](size_t index) const;
    size_t size() const;
    bool has(const std::string& key) const;
    glm::vec3 toVec3() const;

    static JsonValue Parse(const std::string& json);

private:
    static JsonValue parseValue(const std::string& json, size_t& pos);
    static JsonValue parseObject(const std::string& json, size_t& pos);
    static JsonValue parseArray(const std::string& json, size_t& pos);
    static JsonValue parseString(const std::string& json, size_t& pos);
    static JsonValue parseNumber(const std::string& json, size_t& pos);
    static void skipWhitespace(const std::string& json, size_t& pos);
};

class SceneSerializer {
public:
    static bool Save(const std::string& filepath, Coordinator& coordinator, int entityCount);
    static bool Load(const std::string& filepath, Coordinator& coordinator, int& entityCount);
};

#endif // MIST_SCENE_SERIALIZER_H
