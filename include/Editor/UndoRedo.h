#pragma once
#ifndef MIST_UNDO_REDO_H
#define MIST_UNDO_REDO_H

#include <memory>
#include <vector>
#include <string>
#include <functional>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

// Generic lambda-based command
class LambdaCommand : public ICommand {
public:
    LambdaCommand(std::string desc, std::function<void()> exec, std::function<void()> undo)
        : m_Description(std::move(desc)), m_Execute(std::move(exec)), m_Undo(std::move(undo)) {}

    void Execute() override { m_Execute(); }
    void Undo() override { m_Undo(); }
    std::string GetDescription() const override { return m_Description; }

private:
    std::string m_Description;
    std::function<void()> m_Execute;
    std::function<void()> m_Undo;
};

class UndoRedoManager {
public:
    void ExecuteCommand(std::unique_ptr<ICommand> command) {
        command->Execute();
        m_UndoStack.push_back(std::move(command));
        m_RedoStack.clear();

        // Limit stack size
        if (m_UndoStack.size() > m_MaxHistory) {
            m_UndoStack.erase(m_UndoStack.begin());
        }
    }

    void Undo() {
        if (m_UndoStack.empty()) return;
        auto cmd = std::move(m_UndoStack.back());
        m_UndoStack.pop_back();
        cmd->Undo();
        m_RedoStack.push_back(std::move(cmd));
    }

    void Redo() {
        if (m_RedoStack.empty()) return;
        auto cmd = std::move(m_RedoStack.back());
        m_RedoStack.pop_back();
        cmd->Execute();
        m_UndoStack.push_back(std::move(cmd));
    }

    bool CanUndo() const { return !m_UndoStack.empty(); }
    bool CanRedo() const { return !m_RedoStack.empty(); }

    std::string GetUndoDescription() const {
        return m_UndoStack.empty() ? "" : m_UndoStack.back()->GetDescription();
    }
    std::string GetRedoDescription() const {
        return m_RedoStack.empty() ? "" : m_RedoStack.back()->GetDescription();
    }

    void Clear() {
        m_UndoStack.clear();
        m_RedoStack.clear();
    }

    size_t GetUndoCount() const { return m_UndoStack.size(); }
    size_t GetRedoCount() const { return m_RedoStack.size(); }

private:
    std::vector<std::unique_ptr<ICommand>> m_UndoStack;
    std::vector<std::unique_ptr<ICommand>> m_RedoStack;
    size_t m_MaxHistory = 100;
};

#endif
