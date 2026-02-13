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
