include(CMakeParseArguments)
include(ExternalProject)

# 全局属性存储所有模块目标（带前缀）
define_property(GLOBAL PROPERTY ALL_MODULE_TARGETS
    BRIEF_DOCS "List of all third-party module targets"
    FULL_DOCS "List of all third-party module targets created by add_module")

# Brief: 添加模块的函数
# Param：
#   OUTER        - "Engine" 或 "Game"
#   TARGET_NAME  - 模块原始名称（如 SDL3）
#   SOURCE_DIR   - 模块源码目录（包含 CMakeLists.txt）
#   CONFIGURE_ARGS - 可选，传递给 ExternalProject 的 CMAKE_ARGS
# Useage:
# 	file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
# 	foreach(child ${children})
# 	    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child} AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt)
# 	        message(STATUS "[Engine] Adding module: ${child}")
# 	        add_module(
# 	            OUTER "Engine"
# 	            TARGET_NAME "${child}"
# 	            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${child}"
# 	        )
# 	    endif()
# 	endforeach()

function(add_module)
    set(options "")
    set(oneValueArgs OUTER TARGET_NAME SOURCE_DIR)
    set(multiValueArgs CONFIGURE_ARGS)
    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MODULE_OUTER OR NOT MODULE_TARGET_NAME OR NOT MODULE_SOURCE_DIR)
        message(FATAL_ERROR "add_module: OUTER, TARGET_NAME, SOURCE_DIR are required")
    endif()

    # 带前缀的目标名
    set(prefix_target "${MODULE_OUTER}_${MODULE_TARGET_NAME}")
    # 构建目录和安装目录（放在构建树中，避免污染源码）
    set(build_dir "${CMAKE_BINARY_DIR}/third_party/${prefix_target}/build")
    set(install_dir "${CMAKE_BINARY_DIR}/third_party/${prefix_target}/install")

    # 预期库文件名（根据平台）
    if(WIN32)
        set(library_name "${MODULE_TARGET_NAME}.dll")
        set(import_lib_name "${MODULE_TARGET_NAME}.lib")
    elseif(APPLE)
        set(library_name "lib${MODULE_TARGET_NAME}.dylib")
    else()
        set(library_name "lib${MODULE_TARGET_NAME}.so")
    endif()
    set(library_path "${install_dir}/bin/${library_name}")   # Windows DLL 通常在 bin
    if(NOT WIN32)
        set(library_path "${install_dir}/lib/${library_name}")
    endif()
    set(import_lib_path "${install_dir}/lib/${import_lib_name}")

    # 使用 ExternalProject_Add 构建模块
    ExternalProject_Add(${prefix_target}
        SOURCE_DIR ${MODULE_SOURCE_DIR}
        BINARY_DIR ${build_dir}
        INSTALL_DIR ${install_dir}
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${install_dir}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=ON
            ${MODULE_CONFIGURE_ARGS}
        BUILD_ALWAYS OFF   # 只在源码变化时重新构建
        STEP_TARGETS install
    )

    # 创建 IMPORTED 目标
    add_library(${prefix_target} SHARED IMPORTED)
    set_target_properties(${prefix_target} PROPERTIES
        IMPORTED_LOCATION ${library_path}
    )
    if(WIN32 AND EXISTS ${import_lib_path})
        set_target_properties(${prefix_target} PROPERTIES
            IMPORTED_IMPLIB ${import_lib_path}
        )
    endif()

    # 可选：设置头文件路径（假设模块安装头文件到 include 目录）
    set(include_dir "${install_dir}/include")
    if(EXISTS ${include_dir})
        set_target_properties(${prefix_target} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${include_dir}
        )
    endif()

    # 记录到全局列表
    set_property(GLOBAL APPEND PROPERTY ALL_MODULE_TARGETS ${prefix_target})

    # 添加依赖：其他目标链接此模块前，确保已构建
    # 用户可在 target_link_libraries 中直接使用 ${prefix_target}，但需要保证构建顺序
    # 这里不做自动依赖，用户需手动 add_dependencies 或 ExternalProject 会处理
endfunction()

# 函数：统一安装所有模块
function(install_all_modules)
    get_property(module_targets GLOBAL PROPERTY ALL_MODULE_TARGETS)
    foreach(target ${module_targets})
        if(TARGET ${target})
            # 获取 IMPORTED_LOCATION
            get_target_property(loc ${target} IMPORTED_LOCATION)
            get_target_property(implib ${target} IMPORTED_IMPLIB)
            if(loc AND EXISTS ${loc})
                install(FILES ${loc}
                    DESTINATION bin/Modules
                )
                # 如果是 Windows，可能还需要安装 .pdb（可选）
            endif()
            if(WIN32 AND implib AND EXISTS ${implib})
                install(FILES ${implib}
                    DESTINATION lib/Modules
                )
            endif()
            message(STATUS "Scheduled installation for module: ${target}")
        endif()
    endforeach()
endfunction()