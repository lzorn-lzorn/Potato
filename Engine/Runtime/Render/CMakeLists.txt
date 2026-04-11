# ==============================================================================
# Render 模块
# ==============================================================================
# 职责:
#   - 封装渲染后端 (VulkanRHI) 并仅对模块内部开放
#   - 对外暴露 RenderSystem 等高层接口
#   - 管理 HLSL Shader 编译 (通过 Vulkan SDK 的 dxc)
# ==============================================================================
# add_library(Render SHARED VulkanRHI/VulkanContext.cpp)

# ==============================================================================
# 收集源文件
# ==============================================================================
# Render 模块自身的公开头文件
set(RENDER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderSystem.h
)

# VulkanRHI 内部实现（对外隐藏）
file(GLOB_RECURSE VULKANRHI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanRHI/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanRHI/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanRHI/*.h
)

set(RENDER_ALL_SOURCES
    ${RENDER_PUBLIC_HEADERS}
    ${VULKANRHI_SOURCES}
)

# ==============================================================================
# Vulkan SDK
# ==============================================================================
find_package(Vulkan REQUIRED COMPONENTS glslc)


# Vulkan SDK 中包含 dxc，用于编译 HLSL → SPIR-V
# 路径: $VULKAN_SDK/Bin/dxc(.exe)
find_program(VULKAN_DXC_EXECUTABLE
    NAMES dxc
    HINTS "$ENV{VULKAN_SDK}/Bin"
          "${Vulkan_INCLUDE_DIR}/../Bin"
)
if(VULKAN_DXC_EXECUTABLE)
    message(STATUS "[Render] Found dxc (HLSL compiler): ${VULKAN_DXC_EXECUTABLE}")
else()
    message(WARNING "[Render] dxc not found; HLSL shader compilation will not be available.")
endif()

# ==============================================================================
# Shader 目录
# ==============================================================================
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Shader" CACHE PATH "Shader 源文件目录")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Shaders" CACHE PATH "编译后的 SPIR-V 输出目录")

# ==============================================================================
# 创建模块目标 (使用 add_internal_module)
# ==============================================================================
add_internal_module(Render
    TYPE    STATIC
    SOURCES ${RENDER_ALL_SOURCES}

    # 对外暴露 Render/ 目录, 外部通过 "Render/RenderSystem.h" 引用
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/..

    # VulkanRHI 目录仅对本模块内部可见, 对外隐藏实现细节
    PRIVATE_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/VulkanRHI

    # 依赖项:
    #   Vulkan::Vulkan  — Vulkan 运行时库
    #   SoilCore        — 内部基础库
    DEPENDENCY
        Vulkan::Vulkan
		Core::Core

	ALIAS Render::Render
)


target_include_directories(Render PRIVATE ${Vulkan_INCLUDE_DIR})

# ==============================================================================
# 将 dxc / glslc 路径作为编译定义传递给 Render (可选, 便于运行时调用)
# ==============================================================================
if(VULKAN_DXC_EXECUTABLE)
    target_compile_definitions(Render PRIVATE
        RENDER_DXC_PATH="${VULKAN_DXC_EXECUTABLE}"
    )
endif()

if(Vulkan_GLSLC_EXECUTABLE)
    target_compile_definitions(Render PRIVATE
        RENDER_GLSLC_PATH="${Vulkan_GLSLC_EXECUTABLE}"
    )
endif()

# Shader 源目录与输出目录, 供运行时或构建时脚本使用
target_compile_definitions(Render PRIVATE
    RENDER_SHADER_SOURCE_DIR="${SHADER_SOURCE_DIR}"
    RENDER_SHADER_OUTPUT_DIR="${SHADER_OUTPUT_DIR}"
)

# ==============================================================================
# HLSL → SPIR-V 编译函数
# ==============================================================================
# 用法:
#   compile_hlsl_shaders(
#       TARGET  Render
#       SHADERS Shader/triangle.vert.hlsl Shader/triangle.frag.hlsl
#       PROFILES vs_6_0 ps_6_0
#   )
#
# 该函数会为每个 HLSL 文件调用 dxc 编译为 SPIR-V, 并将输出文件
# 作为自定义命令添加到指定的 TARGET 上, 实现增量编译.
function(compile_hlsl_shaders)
    set(oneValueArgs TARGET)
    set(multiValueArgs SHADERS PROFILES)
    cmake_parse_arguments(HLSL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT VULKAN_DXC_EXECUTABLE)
        message(WARNING "[Render] dxc not available, skipping HLSL compilation.")
        return()
    endif()

    list(LENGTH HLSL_SHADERS shader_count)
    list(LENGTH HLSL_PROFILES profile_count)
    if(NOT shader_count EQUAL profile_count)
        message(FATAL_ERROR "[Render] compile_hlsl_shaders: SHADERS 和 PROFILES 数量必须一致")
    endif()

    math(EXPR last_index "${shader_count} - 1")
    foreach(i RANGE 0 ${last_index})
        list(GET HLSL_SHADERS  ${i} shader_file)
        list(GET HLSL_PROFILES ${i} profile)

        get_filename_component(shader_name "${shader_file}" NAME)
        set(output_file "${SHADER_OUTPUT_DIR}/${shader_name}.spv")

        add_custom_command(
            OUTPUT  "${output_file}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT_DIR}"
            COMMAND "${VULKAN_DXC_EXECUTABLE}"
                    -spirv
                    -T ${profile}
                    -E main
                    "${shader_file}"
                    -Fo "${output_file}"
            DEPENDS "${shader_file}"
            COMMENT "[Render] Compiling HLSL: ${shader_name} (${profile}) -> SPIR-V"
            VERBATIM
        )
        list(APPEND SPIRV_OUTPUTS "${output_file}")
    endforeach()

    if(SPIRV_OUTPUTS)
        add_custom_target(${HLSL_TARGET}_Shaders ALL DEPENDS ${SPIRV_OUTPUTS})
        add_dependencies(${HLSL_TARGET} ${HLSL_TARGET}_Shaders)
    endif()
endfunction()
