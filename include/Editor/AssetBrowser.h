#pragma once
#ifndef MIST_ASSET_BROWSER_H
#define MIST_ASSET_BROWSER_H

#include "Core/PathGuard.h"
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <algorithm>

struct AssetEntry {
    std::string name;
    std::string fullPath;
    std::string extension;
    bool isDirectory = false;
    uintmax_t fileSize = 0;
};

class AssetBrowser {
public:
    void SetRootDirectory(const std::string& root) {
        m_RootDir = root;
        m_CurrentDir = root;
        Refresh();
    }

    void Refresh() {
        m_Entries.clear();
        if (!std::filesystem::exists(m_CurrentDir)) return;

        for (auto& entry : std::filesystem::directory_iterator(m_CurrentDir)) {
            AssetEntry asset;
            asset.name = entry.path().filename().string();
            asset.fullPath = entry.path().string();
            asset.isDirectory = entry.is_directory();
            if (!asset.isDirectory) {
                asset.extension = entry.path().extension().string();
                asset.fileSize = entry.file_size();
            }
            m_Entries.push_back(asset);
        }

        // Sort: directories first, then alphabetical
        std::sort(m_Entries.begin(), m_Entries.end(), [](const AssetEntry& a, const AssetEntry& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
    }

    void NavigateTo(const std::string& dir) {
        // Resolve under m_RootDir before accepting. Previously the browser
        // trusted the caller and could end up displaying (and exposing as
        // draggable paths) arbitrary locations on the host's filesystem.
        std::filesystem::path resolved;
        if (!Mist::PathGuard::is_under(m_RootDir, dir, &resolved)) {
            return;
        }
        m_CurrentDir = resolved.generic_string();
        Refresh();
    }

    void NavigateUp() {
        auto parent = std::filesystem::path(m_CurrentDir).parent_path();
        std::filesystem::path resolved;
        if (!Mist::PathGuard::is_under(m_RootDir, parent, &resolved)) {
            return;
        }
        m_CurrentDir = resolved.generic_string();
        Refresh();
    }

    bool CanNavigateUp() const {
        return m_CurrentDir != m_RootDir;
    }

    const std::vector<AssetEntry>& GetEntries() const { return m_Entries; }
    const std::string& GetCurrentDir() const { return m_CurrentDir; }
    const std::string& GetRootDir() const { return m_RootDir; }

    std::string GetRelativePath() const {
        if (m_CurrentDir.find(m_RootDir) == 0) {
            return m_CurrentDir.substr(m_RootDir.size());
        }
        return m_CurrentDir;
    }

    // Filter support
    void SetFilter(const std::string& filter) { m_Filter = filter; }

    std::vector<AssetEntry> GetFilteredEntries() const {
        if (m_Filter.empty()) return m_Entries;
        std::vector<AssetEntry> filtered;
        for (auto& e : m_Entries) {
            if (e.isDirectory || e.name.find(m_Filter) != std::string::npos ||
                e.extension.find(m_Filter) != std::string::npos) {
                filtered.push_back(e);
            }
        }
        return filtered;
    }

    // Callbacks
    std::function<void(const AssetEntry&)> onAssetSelected;
    std::function<void(const AssetEntry&)> onAssetDoubleClicked;

private:
    std::string m_RootDir;
    std::string m_CurrentDir;
    std::string m_Filter;
    std::vector<AssetEntry> m_Entries;
};

#endif
