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

        if ((FrameCounter % 120) == 0) {
            std::cout << "RenderSystem ClearColor: ("
                      << ClearColor.X() << ", "
                      << ClearColor.Y() << ", "
                      << ClearColor.Z() << ")" << std::endl;
        }
    }

    std::string RenderSystem::DescribeBackEnd() const
    {
        if (!Context) {
            return "Unknown";
        }
        return Context->BackendName();
    }

    void RenderSystem::SetClearColor(const Vector3D<float>& color) noexcept
    {
        ClearColor = color;
    }

    Vector3D<float> RenderSystem::GetClearColor() const noexcept
    {
        return ClearColor;
    }
}
