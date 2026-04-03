#include "Core/ModuleManager/ModuleAPI.h"

class PotatoGame {
public:
    bool OnLoad(const PluginHostContext* host) {
        m_host = host;
        if (m_host && m_host->Log) {
            m_host->Log("PotatoGame loaded");
        }
        return true;
    }

    void OnTick(double dt) {
        // Game logic here
    }

    void OnUnload() {
        if (m_host && m_host->Log) {
            m_host->Log("PotatoGame unloaded");
        }
    }

    const char* GetName() const { return "PotatoGame"; }

private:
    const PluginHostContext* m_host = nullptr;
};

static PluginAPI s_api = {};

PLUGIN_API const PluginAPI* Plugin_GetAPI() {
    s_api.abi_version = PLUGIN_ABI_VERSION;
    s_api.struct_size = sizeof(PluginAPI);
    s_api.CreateModule = []() -> void* { return new PotatoGame(); };
    s_api.DestroyModule = [](void* m) { delete static_cast<PotatoGame*>(m); };
    s_api.GetName = [](void* m) -> const char* { return static_cast<PotatoGame*>(m)->GetName(); };
    s_api.OnLoad = [](void* m, const PluginHostContext* h) -> bool { return static_cast<PotatoGame*>(m)->OnLoad(h); };
    s_api.OnTick = [](void* m, double dt) { static_cast<PotatoGame*>(m)->OnTick(dt); };
    s_api.OnUnload = [](void* m) { static_cast<PotatoGame*>(m)->OnUnload(); };
    return &s_api;
}
