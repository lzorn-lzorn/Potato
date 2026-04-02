add_subdirectory(Vulkan)
add_subdirectory(Render)
add_subdirectory(Core)

add_library(Runtime STATIC
	Runtime.cpp
)

target_include_directories(Runtime
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Runtime
	PUBLIC RuntimeRender
	PRIVATE Core
)

# ================================================================================================
# AVX 相关设置
# ================================================================================================
option(USE_AVX2 "Enable AVX2 instructions" ON)

if (USE_AVX2)
    target_compile_options(Runtime PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2>
    )
    target_compile_definitions(Runtime PRIVATE USE_AVX2=1)
endif()
#target_include_directories(Runtime PRIVATE ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# ================================================================================================
# Linux/macOS 需要 pthread
# ================================================================================================
find_package(Threads REQUIRED)
target_link_libraries(Runtime PRIVATE Threads::Threads)