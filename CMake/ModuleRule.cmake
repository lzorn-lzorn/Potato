include(CMakeParseArguments)

function(add_plugin TARGET_NAME)
    set(options)
    set(one_value_args)
    set(multi_value_args SOURCES)

	# from CMakeParseArguments; 在 cmake 3.5 之后变为内置命令
	# 这里的 ARGN 是指 add_plugin() 后面传入的参数列表, cmake 会将多传的参数放在 ARGN 中
	# 以实现变长参数的功能
    cmake_parse_arguments(
		POTATO_PLUGIN          # 生成的变量都将以此开头
		"${options}" 
		"${one_value_args}"     # 单值参数，如 VALUE "hello"
		"${multi_value_args}"   # 多值参数，如 FILES a.txt b.txt
		${ARGN}                 # 待解析的参数列表
	)

    if (NOT ASTRING_PLUGIN_SOURCES)
        message(FATAL_ERROR "add_plugin(${TARGET_NAME}) requires SOURCES")
    endif()

    add_library(${TARGET_NAME} SHARED ${ASTRING_PLUGIN_SOURCES})
    target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
    target_compile_definitions(${TARGET_NAME} PRIVATE PLUGIN_EXPORTS=1)
    set_target_properties(${TARGET_NAME} PROPERTIES
		PREFIX ""
		OUTPUT_NAME "${TARGET_NAME}" # 直接使用 目标名称, 取出 lib_ 前缀
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Plugins"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Plugins"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/Plugins"
    )
endfunction()