

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TARGET_NAME PotatoGame)

# 收集游戏源文件
file(GLOB_RECURSE GAME_SOURCES ${GAME_SOURCE_PATH}/*.cpp)
file(GLOB_RECURSE GAME_HEADERS ${GAME_INCLUDE_PATH}/*.h ${GAME_INCLUDE_PATH}/*.hpp)

add_library(${TARGET_NAME} SHARED
    ${GAME_SOURCES}
    ${GAME_HEADERS}
)

target_include_directories(${TARGET_NAME}
    PRIVATE ${GAME_INCLUDE_PATH}
    PRIVATE ${ENGINE_INCLUDE_PATH} 
)

# 链接引擎 Core 以访问引擎 API
target_link_libraries(${TARGET_NAME}
    PRIVATE Runtime
)

# 导出插件符号
target_compile_definitions(${TARGET_NAME} PRIVATE PLUGIN_EXPORTS=1)

# 输出到 Plugins 目录, 以便引擎运行时加载
set_target_properties(${TARGET_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${TARGET_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>/Plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>/Plugins"
)

# ==============================================================================
# 生成调试符号
# ==============================================================================
if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES
        PDB_NAME "${TARGET_NAME}"
        PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Plugins"
        COMPILE_PDB_NAME "${TARGET_NAME}"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Plugins"
    )
endif()


# ==============================================================================
# 自动发现游戏模块: 每个包含 CMakeLists.txt 的子目录都将被添加
# ==============================================================================
file(GLOB module_children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/Modules ${CMAKE_CURRENT_SOURCE_DIR}/Modules/*)
foreach(module ${module_children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Modules/${module} AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Modules/${module}/CMakeLists.txt)
        message(STATUS "[Game] Adding module: ${module}")
        add_subdirectory(Modules/${module})
    endif()
endforeach()


# ==============================================================================
# 安装头文件和库文件
# ==============================================================================
install(DIRECTORY ${GAME_INCLUDE_PATH}/
    DESTINATION include/${TARGET_NAME}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}
    RUNTIME DESTINATION bin/Plugins      # .dll
    LIBRARY DESTINATION lib/Plugins      # .so/.dylib
    ARCHIVE DESTINATION lib/Plugins      # .lib (Windows)
)

# ==============================================================================
# 导出CMake配置方便 find_package 查找
# ==============================================================================
# install(EXPORT ${TARGET_NAME}
#     FILE ${TARGET_NAME}Config.cmake
#     NAMESPACE ${TARGET_NAME}::
#     DESTINATION lib/cmake/${TARGET_NAME}
# )

# ==============================================================================
# 生成并安装 ConfigVersion.cmake 以支持版本检查
# ==============================================================================
# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake"
#     VERSION ${PROJECT_VERSION}
#     COMPATIBILITY AnyNewerVersion
# )
# install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake"
#     DESTINATION lib/cmake/${TARGET_NAME}
# )

