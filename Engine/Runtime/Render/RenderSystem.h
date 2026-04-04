#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../Vulkan/VulkanContext.hpp"
#include "../Core/Math/Vector.h"

namespace Runtime::Render
{
    class RenderSystem
    {
    public:
        explicit RenderSystem(Vulkan::ContextPtr vulkanContext);

        void DrawFrame();
        [[nodiscard]] std::string DescribeBackEnd() const;
        void SetClearColor(const Core::Math::Vector3D<float>& color) noexcept;
        [[nodiscard]] Core::Math::Vector3D<float> GetClearColor() const noexcept;

    private:
        Vulkan::ContextPtr Context;
        uint64_t FrameCounter = 0;
        Core::Math::Vector3D<float> ClearColor = Core::Math::Vector3D<float>::ZeroVector();
    };
}
