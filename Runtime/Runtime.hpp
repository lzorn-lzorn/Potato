#pragma once

#include <memory>
#include <string>

#include "Render/RenderSystem.hpp"
#include "Vulkan/VulkanContext.hpp"


namespace Runtime
{
	Vulkan::ContextPtr CreateVulkanContext();

	std::unique_ptr<Render::RenderSystem> CreateRenderSystem(const Vulkan::ContextPtr& context);

	std::string GetGreeting();
}
