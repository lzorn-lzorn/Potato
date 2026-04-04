#include "ModuleManager.h"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

PluginManager::PluginManager(PluginHostContext host_context)
    : m_host_context(host_context)
{
}

PluginManager::~PluginManager()
{
    UnloadAll();
}

void PluginManager::Log(const std::string& message) const
{
    if (m_host_context.Log) {
        m_host_context.Log(message.c_str());
    }
}

bool PluginManager::LoadFromDirectory(const std::filesystem::path& directory)
{
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        Log("Plugin directory not found: " + directory.string());
        return true;
    }

    bool all_ok = true;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;

        const auto& path = entry.path();
        auto ext = path.extension().string();

#ifdef _WIN32
        if (ext != ".dll") continue;
#elif defined(__APPLE__)
        if (ext != ".dylib") continue;
#else
        if (ext != ".so") continue;
#endif

        if (!LoadPluginFile(path)) {
            all_ok = false;
        }
    }
    return all_ok;
}

bool PluginManager::LoadPluginFile(const std::filesystem::path& file_path)
{
    Log("Loading plugin: " + file_path.filename().string());

#ifdef _WIN32
    void* handle = static_cast<void*>(LoadLibraryW(file_path.wstring().c_str()));
#else
    void* handle = dlopen(file_path.string().c_str(), RTLD_NOW);
#endif

    if (!handle) {
        Log("Failed to load: " + file_path.filename().string());
        return false;
    }

#ifdef _WIN32
    auto get_api = reinterpret_cast<GetPluginApiFn>(
        GetProcAddress(static_cast<HMODULE>(handle), "Plugin_GetAPI"));
#else
    auto get_api = reinterpret_cast<GetPluginApiFn>(dlsym(handle, "Plugin_GetAPI"));
#endif

    if (!get_api) {
        Log("No Plugin_GetAPI in: " + file_path.filename().string());
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return false;
    }

    const PluginAPI* api = get_api();
    if (!api || api->abi_version != PLUGIN_ABI_VERSION) {
        Log("ABI version mismatch in: " + file_path.filename().string());
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return false;
    }

    void* instance = api->CreateModule();
    if (!instance) {
        Log("CreateModule failed in: " + file_path.filename().string());
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return false;
    }

    if (!api->OnLoad(instance, &m_host_context)) {
        Log("OnLoad failed in: " + file_path.filename().string());
        api->DestroyModule(instance);
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return false;
    }

    LoadedPlugin plugin{};
    plugin.module_handle = handle;
    plugin.module_instance = instance;
    plugin.api = api;
    plugin.file_name = file_path.filename().string();
    m_plugins.push_back(plugin);

    Log("Loaded: " + std::string(api->GetName(instance)));
    return true;
}

void PluginManager::TickAll(double delta_time_seconds)
{
    for (auto& plugin : m_plugins) {
        if (plugin.api && plugin.module_instance) {
            plugin.api->OnTick(plugin.module_instance, delta_time_seconds);
        }
    }
}

void PluginManager::UnloadAll()
{
    for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
        if (it->api && it->module_instance) {
            it->api->OnUnload(it->module_instance);
            it->api->DestroyModule(it->module_instance);
        }
        if (it->module_handle) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(it->module_handle));
#else
            dlclose(it->module_handle);
#endif
        }
    }
    m_plugins.clear();
}

std::size_t PluginManager::GetLoadedCount() const noexcept
{
    return m_plugins.size();
}
