#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../Vulkan/VulkanContext.hpp"

namespace Runtime::Render
{
    class RenderSystem
    {
    public:
        explicit RenderSystem(Vulkan::ContextPtr vulkanContext);

        void DrawFrame();
        [[nodiscard]] std::string DescribeBackEnd() const;

    private:
        Vulkan::ContextPtr Context;
        uint64_t FrameCounter = 0;
    };
}
