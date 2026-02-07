cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# note: 启用生成 compile_commands.json
set(EXPORT_COMPILE_COMMANDS ON)

project(MyProject            # 项目名称
    LANGUAGES CXX C          # 使用语言
    DESCRIPTION "Patoto!"    # 项目描述信息
    VERSION 0.1.0            # 项目版本    
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/$<CONFIGURATION>")


add_subdirectory(Runtime)
add_subdirectory(Editor)