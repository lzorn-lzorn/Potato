#include "VulkanContext.hpp"

#include <iostream>

namespace Runtime::Vulkan
{
    Context::Context() = default;

    Context::~Context()
    {
        if (bInitialized) {
            Shutdown();
        }
    }

    bool Context::Initialize()
    {
        // TODO: Wire up real Vulkan bootstrap here.
        bInitialized = true;
        std::cout << "[RuntimeVulkan] Context initialized" << std::endl;
        return bInitialized;
    }

    void Context::Shutdown()
    {
        if (!bInitialized) {
            return;
        }
        bInitialized = false;
        std::cout << "[RuntimeVulkan] Context shutdown" << std::endl;
    }

    std::string Context::BackendName() const
    {
        return "Vulkan";
    }
}
