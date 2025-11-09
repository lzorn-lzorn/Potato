cmake_minimum_required(VERSION 3.16)
project(Core LANGUAGES CXX)

add_library(Core INTERFACE)

# 如果生成动态库在 window 下要开启 WINDOWS_EXPORT_ALL_SYMBOLS 不然 动态库会无法使用
# if(WIN32)
#     set(WINDOWS_EXPORT_ALL_SYMBOLS ON)
# endif()

# target_include_directories(Base INTERFACE
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
#     $<INSTALL_INTERFACE:Include>
#     # include 路径下有个 Base 文件夹
#     # 此时别人应用时可以 #include <Base/Number.hpp>
# )

# 安装头文件
# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION include
#     FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
# )

# 安装库目标
# install (TARGETS ... 
#     ARCHIVE DESTINATION lib # 对于静态库, 拷贝到 lib 下
#     RUNTIME DESTINATION bin # 对于可执行文件, 拷贝到 bin 下
#     LIBRARY DESTINATION lib # 对于动态库, 拷贝到 lib 下
#)
# Windows 下 LIBRARY 是没有用的, 因为 Windows 要求 dll 和 exe 在同一个文件夹
# install(TARGETS Base EXPORT BaseTargets)

# install(FILES Number.hpp String.hpp
#     DESTINATION Include # 把 Number.hpp 安装到 include 目录
# )

# 安装文档
# install(FILES Readme.md
#     DESTINATION Doc # 把 Readme.md 安装到 Doc 目录
# )

# 导出targets为cmake包
# install(EXPORT BaseTargets
#     NAMESPACE Base:: # 这里导出为 Base::Base
#     DESTINATION Lib/Cmake/Base
# )

# # 生成Config文件
# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/BaseConfigVersion.cmake"
#     VERSION 1.0.0
#     COMPATIBILITY AnyNewerVersion
# )
# configure_package_config_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BaseConfig.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/BaseConfig.cmake"
#     INSTALL_DESTINATION Lib/Cmake/Base
# )
# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/BaseConfig.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/BaseConfigVersion.cmake"
#     DESTINATION Lib/Cmake/Base
# )
