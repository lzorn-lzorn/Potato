#pragma once

#include <cstdint>

#if defined(_WIN32)
#if defined(PLUGIN_EXPORTS)
#define PLUGIN_API extern "C" __declspec(dllexport)
#else
#define PLUGIN_API extern "C"
#endif
#else
#if defined(PLUGIN_EXPORTS)
#define PLUGIN_API extern "C" __attribute__((visibility("default")))
#else
#define PLUGIN_API extern "C"
#endif
#endif

constexpr std::uint32_t PLUGIN_ABI_VERSION = 2;

struct PluginHostContext {
    void (*Log)(const char* message);
};

struct PluginAPI {
    std::uint32_t abi_version;
    std::uint32_t struct_size;

    void* (*CreateModule)();
    void (*DestroyModule)(void* module_instance);

    const char* (*GetName)(void* module_instance);
    bool (*OnLoad)(void* module_instance, const PluginHostContext* host_context);
    void (*OnTick)(void* module_instance, double delta_time_seconds);
    void (*OnUnload)(void* module_instance);
};

using GetPluginApiFn = const PluginAPI* (*)();

PLUGIN_API const PluginAPI* Plugin_GetAPI();