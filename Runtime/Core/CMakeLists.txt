add_library(Core SHARED
	dummy.cpp
)

target_include_directories(Core
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(Core
	PRIVATE CORE_EXPORTS
)
