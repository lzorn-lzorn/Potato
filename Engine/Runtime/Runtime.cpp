// #include "Runtime.hpp"
// #include "Core/Entity/AABB.h"

// namespace Runtime
// {
//     Vulkan::ContextPtr CreateVulkanContext()
//     {
//         auto context = std::make_shared<Vulkan::Context>();
//         context->Initialize();
//         return context;
//     }

//     std::unique_ptr<Runtime::Render::RenderSystem> CreateRenderSystem(const Vulkan::ContextPtr& context)
//     {
//         return std::make_unique<Runtime::Render::RenderSystem>(context);
//     }

//     std::string GetGreeting()
//     {
//         auto context = CreateVulkanContext();
//         auto renderer = CreateRenderSystem(context);
//         renderer->DrawFrame();
//         return "Hello from " + renderer->DescribeBackEnd();
//     }

    
// }
