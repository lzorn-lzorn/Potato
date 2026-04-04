
file(GLOB_RECURSE ModuleManager_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp)

add_internal_module(ModuleManager
	TYPE OBJECT
	INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
	SOURCES ${ModuleManager_SOURCES}
)
