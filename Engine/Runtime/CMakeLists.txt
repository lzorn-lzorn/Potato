
set(TARGET_NAME Runtime)

add_subdirectory(Core)

# 收集 Runtime 自身的源文件(如果需要)
# file(GLOB_RECURSE RUNTIME_SOURCE ${ENGINE_RUNTIME_PATH}/Source/*.cpp)
# file(GLOB_RECURSE RUNTIME_HEADERS ${ENGINE_RUNTIME_PATH}/Include/*.h ${ENGINE_RUNTIME_PATH}/Include/*.hpp)

add_library(${TARGET_NAME} SHARED
    Runtime.cpp
    # ${RUNTIME_SOURCE}
    # ${RUNTIME_HEADERS}
)

target_link_libraries(${TARGET_NAME}
    PRIVATE Core::Core
    PRIVATE SDL3::SDL3
)

# 让链接 Runtime 的目标能包含 "Core/xxx.h" 格式的头文件
target_include_directories(Runtime PUBLIC ${ENGINE_ROOT_PATH}/Runtime)

# ==============================================================================
# Linux/macOS 需要 pthread
# ==============================================================================
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE Threads::Threads)

# ==============================================================================
# AVX 相关设置
# ==============================================================================
if (IS_ENABLE_AVX2)
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2>
    )
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_AVX2=1)
endif()

# ==============================================================================
# Vulkan 设置
# ==============================================================================
find_package(Vulkan QUIET)

if (Vulkan_FOUND)
    target_link_libraries(${TARGET_NAME}
        PUBLIC Vulkan::Vulkan
    )
    target_compile_definitions(${TARGET_NAME}
        PUBLIC RUNTIME_HAS_VULKAN=1
    )
else()
    message(WARNING "Vulkan SDK not found; RuntimeVulkan built with stub implementations.")
    target_compile_definitions(${TARGET_NAME}
        PUBLIC RUNTIME_HAS_VULKAN=0
    )
endif()

# 加入全局模块列表
set_property(GLOBAL APPEND PROPERTY INTERNAL_MODULES_LIST Runtime)