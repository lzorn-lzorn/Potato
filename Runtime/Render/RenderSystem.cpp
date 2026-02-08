#include "RenderSystem.hpp"

#include <iostream>
#include <utility>

namespace Runtime::Render
{
    RenderSystem::RenderSystem(Vulkan::ContextPtr vulkanContext)
        : Context(std::move(vulkanContext))
    {
        if (Context && !Context->IsInitialized()) {
            Context->Initialize();
        }
    }

    void RenderSystem::DrawFrame()
    {
        if (!Context || !Context->IsInitialized()) {
            std::cerr << "RenderSystem: Vulkan context missing" << std::endl;
            return;
        }

        ++FrameCounter;
    }

    std::string RenderSystem::DescribeBackEnd() const
    {
        if (!Context) {
            return "Unknown";
        }
        return Context->BackendName();
    }
}
