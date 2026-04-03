#pragma once

#include "ModuleAPI.h"

#include <filesystem>
#include <string>
#include <vector>

class PluginManager {
public:
    explicit PluginManager(PluginHostContext host_context);
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    bool LoadFromDirectory(const std::filesystem::path& directory);
    void TickAll(double delta_time_seconds);
    void UnloadAll();

    [[nodiscard]] std::size_t GetLoadedCount() const noexcept;

private:
    bool LoadPluginFile(const std::filesystem::path& file_path);
    void Log(const std::string& message) const;

private:
    struct LoadedPlugin {
        void* module_handle = nullptr;
        void* module_instance = nullptr;
        const PluginAPI* api = nullptr;
        std::string file_name;
    };

    PluginHostContext m_host_context{};
    std::vector<LoadedPlugin> m_plugins;
};