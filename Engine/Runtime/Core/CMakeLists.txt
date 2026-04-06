

add_subdirectory(Math)
add_subdirectory(ModuleManager)

add_library(Core STATIC dummy.cpp)


# 内部的模块使用了 add_internal_module 则不需要手动 target_include_directories
# 只需要 target_link_libraries 链接模块即可, 其包含目录会自动传播到依赖者
target_include_directories(Core
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Core.h
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CoreImpl.h
)

# 一个模块是否在某个名称空间下不是由他自己决定的, 而是由父目录来决定的
# 也就是说当 Math 在 Core/ 时候, 链接 Math 就必须 Core::Math 
# 这个操作是在 Core/CMakeLists.txt 中决定好的, 不是在 Math add_internal_module 时指定的

# 为每个子模块创建命名空间别名(Core::前缀)
add_library(Core::Math ALIAS Math)
add_library(Core::ModuleManager ALIAS ModuleManager)

# 这里为了将 Core 链接为一个完整的库(dll)
# 如果子模块是动态库 SHARED, 则会引入多余的依赖 (例如 Math.dll), 需要运行时保证 dll 必须存在
# 如果子模块是静态库 STATIC, 则会使得 Core.dll 依赖该静态库
# 如果子模块是 head-only 接口库 INTERFACE, 虽然不会合并但是会传递依赖
# 如果子模块是对象库 OBJECT, 则会完美合并
# target_link_libraries(Core
#	 INTERFACE Math head-only 接口库 INTERFACE 传递依赖, 但不合并
# 	 PRIVATE Math
# )
target_link_libraries(Core
	PUBLIC Core::Math 
	PUBLIC Core::ModuleManager
)

# 为 Core 本身创建一个别名 Core::Core
add_library(Core::Core ALIAS Core)

 
# 将 Core 也加入全局模块列表 INTERNAL_MODULES_LIST,
# 此时上层可以通过
# 	get_property(all_modules GLOBAL PROPERTY INTERNAL_MODULES_LIST)
# 	message(STATUS "All internal modules: ${all_modules}")
# 链接
set_property(GLOBAL APPEND PROPERTY INTERNAL_MODULES_LIST Core::Core)