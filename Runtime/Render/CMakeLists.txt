cmake_minimum_required(VERSION 3.20)

add_library(RuntimeRender STATIC
    RenderSystem.cpp
)

target_include_directories(RuntimeRender
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(RuntimeRender
    PUBLIC RuntimeVulkan
    PUBLIC Core
)
