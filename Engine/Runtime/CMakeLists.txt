
set(TARGET_NAME Runtime)

file(GLOB_RECURSE RUNTIME_SOURCE ${ENGINE_RUNTIME_PATH}/Source/*.cpp)
file(GLOB_RECURSE RUNTIME_HEADERS ${ENGINE_RUNTIME_PATH}/Include/*.h ${ENGINE_RUNTIME_PATH}/Include/*.hpp)

add_library(${TARGET_NAME} SHARED
    ${RUNTIME_SOURCE}
    ${RUNTIME_HEADERS}
)

target_include_directories(${TARGET_NAME} PUBLIC ${ENGINE_RUNTIME_PATH}/Include)

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
#target_include_directories(Runtime PRIVATE ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# ==============================================================================
# Linux/macOS 需要 pthread
# ==============================================================================
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE Threads::Threads)

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

# ==============================================================================
# Editor 可执行文件 (SDL 主入口)
# ==============================================================================
if(IS_ENABLE_EDITOR)
    
endif()
