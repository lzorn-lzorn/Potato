add_subdirectory(Modules)
add_subdirectory(Runtime)
add_subdirectory(Launcher)

# 输出到 Plugins 目录, 以便引擎运行时加载
set_target_properties(${TARGET_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${TARGET_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Engine"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>/Engine"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>/Engine"
)

# ==============================================================================
# 路径安装与打包
# ==============================================================================
install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION bin/Engine
    LIBRARY DESTINATION lib/Engine
    ARCHIVE DESTINATION lib/Engine
)
install(DIRECTORY ${ENGINE_RUNTIME_PATH}/Include/ DESTINATION include/Runtime
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# ==============================================================================
# 生成调试符号
# ==============================================================================
if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES
        PDB_NAME "${TARGET_NAME}"
        PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Engine"
        COMPILE_PDB_NAME "${TARGET_NAME}"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/Engine"
    )
endif()
