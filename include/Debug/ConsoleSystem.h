#pragma once
#ifndef MIST_CONSOLE_SYSTEM_H
#define MIST_CONSOLE_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>

class ConsoleSystem {
public:
    using CommandHandler = std::function<std::string(const std::vector<std::string>& args)>;

    void RegisterCommand(const std::string& name, CommandHandler handler, const std::string& help = "") {
        m_Commands[name] = {handler, help};
    }

    std::string Execute(const std::string& cmdLine) {
        auto tokens = tokenize(cmdLine);
        if (tokens.empty()) return "";

        std::string cmd = tokens[0];
        tokens.erase(tokens.begin());

        auto it = m_Commands.find(cmd);
        if (it == m_Commands.end()) {
            return "Unknown command: " + cmd;
        }

        std::string result = it->second.handler(tokens);
        m_History.push_back("> " + cmdLine);
        if (!result.empty()) m_History.push_back(result);
        return result;
    }

    void RegisterBuiltins() {
        RegisterCommand("help", [this](const std::vector<std::string>&) {
            std::string result = "Available commands:\n";
            for (auto& [name, entry] : m_Commands) {
                result += "  " + name;
                if (!entry.help.empty()) result += " - " + entry.help;
                result += "\n";
            }
            return result;
        }, "Show available commands");

        RegisterCommand("clear", [this](const std::vector<std::string>&) {
            m_History.clear();
            return std::string();
        }, "Clear console history");

        RegisterCommand("echo", [](const std::vector<std::string>& args) {
            std::string result;
            for (auto& a : args) { result += a + " "; }
            return result;
        }, "Echo text");
    }

    const std::vector<std::string>& GetHistory() const { return m_History; }

    void AddLog(const std::string& message) {
        m_History.push_back(message);
        if (m_History.size() > m_MaxHistory) {
            m_History.erase(m_History.begin());
        }
    }

    std::vector<std::string> GetCompletions(const std::string& partial) const {
        std::vector<std::string> completions;
        for (auto& [name, entry] : m_Commands) {
            if (name.find(partial) == 0) completions.push_back(name);
        }
        return completions;
    }

private:
    struct CommandEntry {
        CommandHandler handler;
        std::string help;
    };

    std::unordered_map<std::string, CommandEntry> m_Commands;
    std::vector<std::string> m_History;
    size_t m_MaxHistory = 1000;

    std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream stream(line);
        std::string token;
        while (stream >> token) tokens.push_back(token);
        return tokens;
    }
};

#endif
