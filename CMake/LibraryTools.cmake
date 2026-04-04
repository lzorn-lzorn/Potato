# 辅助函数：为源文件生成 IDE 分组（按目录结构）
function(internal_source_group target_name sources)
    foreach(src IN LISTS sources)
        get_filename_component(src_dir "${src}" DIRECTORY)
        if(src_dir)
            string(REPLACE "/" "\\" group_name "${src_dir}")
            source_group("${group_name}" FILES "${src}")
        else()
            source_group("" FILES "${src}")
        endif()
    endforeach()
endfunction()

# 辅助函数：检查列表中的重复项
function(list_assert_duplicates input_list)
    list(LENGTH input_list len)
    if(len LESS 2)
        return()
    endif()
    set(unique_list ${input_list})
    list(REMOVE_DUPLICATES unique_list)
    list(LENGTH unique_list unique_len)
    if(NOT len EQUAL unique_len)
        message(FATAL_ERROR "Duplicate items found in list")
    endif()
endfunction()

# 主函数：添加内部模块
# 调用格式：
#   add_internal_module(<name>
#       TYPE      <STATIC|SHARED|OBJECT|INTERFACE>   # 可选，默认为 STATIC
#       SOURCES   <src1> [<src2> ...]                # 源文件列表
#       INCLUDES  <inc1> [<inc2> ...]                # 包含目录列表
#       PRIVATE_INCLUDES  <inc1> [<inc2> ...]        # 私有包含目录列表
#       DEPENDENCY <dep1> [<dep2> ...]               # 可选，依赖的其他目标
#   )
function(add_internal_module name)
    set(options "")
    set(oneValueArgs TYPE)
    set(multiValueArgs SOURCES INCLUDES DEPENDENCY PRIVATE_INCLUDES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 默认类型
    if(NOT ARG_TYPE)
        set(ARG_TYPE STATIC)
    endif()

    # 创建库目标
    if(ARG_TYPE STREQUAL "INTERFACE")
        # 接口库不能有源文件
        if(ARG_SOURCES)
            message(FATAL_ERROR "add_internal_module: INTERFACE library ${name} cannot have SOURCES")
        endif()
        add_library(${name} INTERFACE)
    else()
        # 非接口库必须有源文件
        if(NOT ARG_SOURCES)
            message(FATAL_ERROR "add_internal_module: non-INTERFACE library ${name} requires SOURCES")
        endif()
        add_library(${name} ${ARG_TYPE} ${ARG_SOURCES})
    endif()

    # 设置包含目录（PUBLIC 以便依赖者自动获得）
    if(ARG_INCLUDES)
        if(ARG_TYPE STREQUAL "INTERFACE")
            target_include_directories(${name} INTERFACE ${ARG_INCLUDES})
        else()
            target_include_directories(${name} PUBLIC ${ARG_INCLUDES})
        endif()
    endif()

    # 处理公开包含目录（PUBLIC）
    if(ARG_PUBLIC_INCLUDES)
        if(ARG_TYPE STREQUAL "INTERFACE")
            target_include_directories(${name} INTERFACE ${ARG_PUBLIC_INCLUDES})
        else()
            target_include_directories(${name} PUBLIC ${ARG_PUBLIC_INCLUDES})
        endif()
    endif()

    # 设置私有包含目录
    if(ARG_PRIVATE_INCLUDES)
        if(NOT ARG_TYPE STREQUAL "INTERFACE")
            target_include_directories(${name} PRIVATE ${ARG_PRIVATE_INCLUDES})
        else()
            message(WARNING "INTERFACE library ${name} cannot have PRIVATE_INCLUDES, ignored")
        endif()
    endif()

    # 链接依赖库
    if(ARG_DEPENDENCY)
        if(ARG_TYPE STREQUAL "INTERFACE")
            target_link_libraries(${name} INTERFACE ${ARG_DEPENDENCY})
        else()
            target_link_libraries(${name} PUBLIC ${ARG_DEPENDENCY})
        endif()
    endif()

    # 处理循环依赖（仅对非 OBJECT、非 INTERFACE 的静态/动态库）
    if(NOT ARG_TYPE STREQUAL "OBJECT" AND NOT ARG_TYPE STREQUAL "INTERFACE")
        set_property(TARGET ${name} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 3)
    endif()

    # 源文件分组（仅非接口库）
    if(NOT ARG_TYPE STREQUAL "INTERFACE")
        internal_source_group(${name} "${ARG_SOURCES}")
        list_assert_duplicates("${ARG_SOURCES}")
    endif()

    if(ARG_INCLUDES)
        list_assert_duplicates("${ARG_INCLUDES}")
    endif()

    # 全局收集所有内部模块
    set_property(GLOBAL APPEND PROPERTY INTERNAL_MODULES_LIST ${name})
endfunction()

# 辅助函数：获取所有已添加的内部模块列表
function(get_internal_modules out_var)
    get_property(modules GLOBAL PROPERTY INTERNAL_MODULES_LIST)
    set(${out_var} ${modules} PARENT_SCOPE)
endfunction()