#include "Scene/SceneSerializer.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "Core/Logger.h"
#include <fstream>
#include <algorithm>
#include <cmath>

extern Coordinator gCoordinator;

// --- JsonWriter ---

void JsonWriter::writeIndent() {
    for (int i = 0; i < m_Indent; i++) m_Stream << "  ";
}

void JsonWriter::writeComma() {
    if (m_NeedComma) m_Stream << ",";
    m_Stream << "\n";
    m_NeedComma = false;
}

void JsonWriter::BeginObject() {
    if (m_NeedComma) writeComma();
    writeIndent();
    m_Stream << "{";
    m_Indent++;
    m_NeedComma = false;
}

void JsonWriter::EndObject() {
    m_Indent--;
    m_Stream << "\n";
    writeIndent();
    m_Stream << "}";
    m_NeedComma = true;
}

void JsonWriter::BeginArray(const std::string& key) {
    writeComma();
    writeIndent();
    m_Stream << "\"" << key << "\": [";
    m_Indent++;
    m_NeedComma = false;
}

void JsonWriter::EndArray() {
    m_Indent--;
    m_Stream << "\n";
    writeIndent();
    m_Stream << "]";
    m_NeedComma = true;
}

void JsonWriter::Key(const std::string& key) {
    writeComma();
    writeIndent();
    m_Stream << "\"" << key << "\": ";
    m_NeedComma = false;
}

void JsonWriter::String(const std::string& value) {
    m_Stream << "\"" << value << "\"";
    m_NeedComma = true;
}

void JsonWriter::Number(float value) {
    if (std::isfinite(value))
        m_Stream << value;
    else
        m_Stream << "0.0";
    m_NeedComma = true;
}

void JsonWriter::Number(int value) {
    m_Stream << value;
    m_NeedComma = true;
}

void JsonWriter::Bool(bool value) {
    m_Stream << (value ? "true" : "false");
    m_NeedComma = true;
}

void JsonWriter::Vec3(const std::string& key, const glm::vec3& v) {
    writeComma();
    writeIndent();
    m_Stream << "\"" << key << "\": [" << v.x << ", " << v.y << ", " << v.z << "]";
    m_NeedComma = true;
}

// --- JsonValue ---

static JsonValue s_NullValue;

const JsonValue& JsonValue::operator[](const std::string& key) const {
    for (auto& kv : objectValues) {
        if (kv.first == key) return kv.second;
    }
    return s_NullValue;
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (index < arrayValues.size()) return arrayValues[index];
    return s_NullValue;
}

size_t JsonValue::size() const {
    if (type == ARRAY) return arrayValues.size();
    if (type == OBJECT) return objectValues.size();
    return 0;
}

bool JsonValue::has(const std::string& key) const {
    for (auto& kv : objectValues) {
        if (kv.first == key) return true;
    }
    return false;
}

glm::vec3 JsonValue::toVec3() const {
    if (type == ARRAY && arrayValues.size() >= 3) {
        return glm::vec3(arrayValues[0].numValue, arrayValues[1].numValue, arrayValues[2].numValue);
    }
    return glm::vec3(0.0f);
}

void JsonValue::skipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t'))
        pos++;
}

JsonValue JsonValue::parseString(const std::string& json, size_t& pos) {
    JsonValue val;
    val.type = STRING;
    pos++; // skip opening "
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\') {
            pos++;
            if (pos < json.size()) {
                switch (json[pos]) {
                    case '"':  val.strValue += '"'; break;
                    case '\\': val.strValue += '\\'; break;
                    case 'n':  val.strValue += '\n'; break;
                    case 't':  val.strValue += '\t'; break;
                    default:   val.strValue += json[pos]; break;
                }
            }
        } else {
            val.strValue += json[pos];
        }
        pos++;
    }
    if (pos < json.size()) pos++; // skip closing "
    return val;
}

JsonValue JsonValue::parseNumber(const std::string& json, size_t& pos) {
    JsonValue val;
    val.type = NUMBER;
    size_t start = pos;
    if (pos < json.size() && json[pos] == '-') pos++;
    while (pos < json.size() && (isdigit(json[pos]) || json[pos] == '.' || json[pos] == 'e' || json[pos] == 'E' || json[pos] == '+' || json[pos] == '-')) {
        if ((json[pos] == '+' || json[pos] == '-') && pos > start && json[pos-1] != 'e' && json[pos-1] != 'E') break;
        pos++;
    }
    val.numValue = std::stof(json.substr(start, pos - start));
    return val;
}

JsonValue JsonValue::parseArray(const std::string& json, size_t& pos) {
    JsonValue val;
    val.type = ARRAY;
    pos++; // skip [
    skipWhitespace(json, pos);
    while (pos < json.size() && json[pos] != ']') {
        val.arrayValues.push_back(parseValue(json, pos));
        skipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ',') pos++;
        skipWhitespace(json, pos);
    }
    if (pos < json.size()) pos++; // skip ]
    return val;
}

JsonValue JsonValue::parseObject(const std::string& json, size_t& pos) {
    JsonValue val;
    val.type = OBJECT;
    pos++; // skip {
    skipWhitespace(json, pos);
    while (pos < json.size() && json[pos] != '}') {
        skipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != '"') break;
        JsonValue keyVal = parseString(json, pos);
        skipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ':') pos++;
        skipWhitespace(json, pos);
        JsonValue value = parseValue(json, pos);
        val.objectValues.push_back({keyVal.strValue, value});
        skipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ',') pos++;
        skipWhitespace(json, pos);
    }
    if (pos < json.size()) pos++; // skip }
    return val;
}

JsonValue JsonValue::parseValue(const std::string& json, size_t& pos) {
    skipWhitespace(json, pos);
    if (pos >= json.size()) return JsonValue();

    char c = json[pos];
    if (c == '"') return parseString(json, pos);
    if (c == '{') return parseObject(json, pos);
    if (c == '[') return parseArray(json, pos);
    if (c == 't' || c == 'f') {
        JsonValue val;
        val.type = BOOL;
        if (json.substr(pos, 4) == "true") { val.boolValue = true; pos += 4; }
        else { val.boolValue = false; pos += 5; }
        return val;
    }
    if (c == 'n') { pos += 4; return JsonValue(); }
    if (c == '-' || isdigit(c)) return parseNumber(json, pos);
    return JsonValue();
}

JsonValue JsonValue::Parse(const std::string& json) {
    size_t pos = 0;
    return parseValue(json, pos);
}

// --- SceneSerializer ---

bool SceneSerializer::Save(const std::string& filepath, Coordinator& coordinator, int entityCount) {
    JsonWriter w;
    w.BeginObject();

    w.Key("version"); w.String("0.4.0");
    w.Key("engine"); w.String("MistEngine");

    w.BeginArray("entities");

    for (int i = 0; i < entityCount; i++) {
        Entity entity = static_cast<Entity>(i);

        // Check if entity has at least a transform
        bool hasTransform = false;
        TransformComponent transform;
        try {
            transform = coordinator.GetComponent<TransformComponent>(entity);
            hasTransform = true;
        } catch (...) {}

        if (!hasTransform) continue;

        w.BeginObject();
        w.Key("id"); w.Number(static_cast<int>(entity));

        // Transform
        w.Vec3("position", transform.position);
        w.Vec3("rotation", transform.rotation);
        w.Vec3("scale", transform.scale);

        // Render component
        try {
            auto& render = coordinator.GetComponent<RenderComponent>(entity);
            w.Key("visible"); w.Bool(render.visible);
            w.Key("hasRenderable"); w.Bool(render.renderable != nullptr);
        } catch (...) {}

        // Physics component
        try {
            auto& physics = coordinator.GetComponent<PhysicsComponent>(entity);
            w.Key("hasPhysics"); w.Bool(true);
            w.Key("syncTransform"); w.Bool(physics.syncTransform);
            if (physics.rigidBody) {
                w.Key("mass"); w.Number(physics.rigidBody->getMass());
            }
        } catch (...) {}

        w.EndObject();
    }

    w.EndArray();
    w.EndObject();

    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for writing: ", filepath);
        return false;
    }
    file << w.GetString();
    file.close();

    LOG_INFO("Scene saved to: ", filepath);
    return true;
}

bool SceneSerializer::Load(const std::string& filepath, Coordinator& coordinator, int& entityCount) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for reading: ", filepath);
        return false;
    }

    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    JsonValue root = JsonValue::Parse(json);
    if (root.type != JsonValue::OBJECT) {
        LOG_ERROR("Invalid scene file format");
        return false;
    }

    if (!root.has("entities")) {
        LOG_ERROR("Scene file has no entities array");
        return false;
    }

    const JsonValue& entities = root["entities"];
    entityCount = 0;

    for (size_t i = 0; i < entities.size(); i++) {
        const JsonValue& ent = entities[i];

        Entity entity = coordinator.CreateEntity();
        entityCount = std::max(entityCount, static_cast<int>(entity) + 1);

        // Transform
        TransformComponent transform;
        if (ent.has("position")) transform.position = ent["position"].toVec3();
        if (ent.has("rotation")) transform.rotation = ent["rotation"].toVec3();
        if (ent.has("scale")) transform.scale = ent["scale"].toVec3();
        coordinator.AddComponent(entity, transform);

        // Render (just set visible flag; actual meshes need recreation)
        if (ent.has("visible")) {
            RenderComponent render;
            render.renderable = nullptr;
            render.visible = ent["visible"].boolValue;
            coordinator.AddComponent(entity, render);
        }

        // Physics (flag only; actual rigid bodies need recreation)
        if (ent.has("hasPhysics") && ent["hasPhysics"].boolValue) {
            PhysicsComponent physics;
            physics.rigidBody = nullptr;
            physics.syncTransform = ent.has("syncTransform") ? ent["syncTransform"].boolValue : true;
            coordinator.AddComponent(entity, physics);
        }
    }

    LOG_INFO("Scene loaded from: ", filepath, " (", entities.size(), " entities)");
    return true;
}
