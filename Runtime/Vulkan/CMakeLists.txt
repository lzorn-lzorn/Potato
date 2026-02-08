cmake_minimum_required(VERSION 3.20)

add_library(RuntimeVulkan STATIC
    VulkanContext.cpp
)

find_package(Vulkan QUIET)

if (Vulkan_FOUND)
    target_link_libraries(RuntimeVulkan
        PUBLIC Vulkan::Vulkan
    )
    target_compile_definitions(RuntimeVulkan
        PUBLIC RUNTIME_HAS_VULKAN=1
    )
else()
    message(WARNING "Vulkan SDK not found; RuntimeVulkan built with stub implementations.")
    target_compile_definitions(RuntimeVulkan
        PUBLIC RUNTIME_HAS_VULKAN=0
    )
endif()

target_include_directories(RuntimeVulkan
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
