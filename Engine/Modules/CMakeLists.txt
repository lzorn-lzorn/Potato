# ==============================================================================
# 自动发现引擎模块: 每个包含 CMakeLists.txt 的子目录都将被添加
# ==============================================================================
include(${CMAKE_SOURCE_DIR}/CMake/ModuleRule.cmake)

# 设置SDL字体库
set(SDLTTF_VENDORED ON)

file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(child ${children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child} AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt)
        message(STATUS "[Engine] Adding module: ${child}")
        add_subdirectory(${child})
    endif()
endforeach()
