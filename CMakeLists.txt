cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project(POTATO               # 项目名称
    LANGUAGES CXX C          # 使用语言
    DESCRIPTION "Patoto!"    # 项目描述信息
    VERSION 0.1.0            # 项目版本    
)

# $<CONFIGURATION> 会用于区分 Debug/Release 等不同构建类型的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>)

set(ENGINE_RUNTIME_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Runtime)
set(ENGINE_MODULES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Modules)
set(GAME_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source)
set(GAME_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Game/Include)

#===============================================================================
# 构建选项
#===============================================================================
option(IS_ENABLE_EDITOR "是否启用编辑器" OFF)
option(IS_WITH_GAME "是否连同游戏一起构建" ON)
option(IS_ENABLE_VULKAN "是个否启用 Vulkan, 不启用则使用 OpenGL" ON)
option(IS_ENABLE_AVX2 "是否启用 AVX2 指令集优化" ON)
option(IS_RECOMPILE_MODULES "是否强制编译所有模块" ON)

# ==============================================================================
# CMake 模块路径
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindCMakeConfig")
include(ModuleRule)

# ==============================================================================
# 构建引擎
# ==============================================================================
add_subdirectory(Engine)

# ==============================================================================
# 构建游戏 (作为 DLL 被引擎加载)
# ==============================================================================
if(IS_WITH_GAME)
    add_subdirectory(Game)
endif()


