add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

add_executable(Editor
	Main.cpp
)

target_link_libraries(Editor
PRIVATE 
	Runtime
	SDL3::SDL3
)
