add_library(Runtime STATIC
	Runtime.cpp
)

target_include_directories(Runtime
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
