
# 如何正确的添加子模块
Game 模块链接的引擎模块默认是 Runtime 模块, 引擎也期望 Game 模块只链接 Runtime 模块.

Runtime 下的一级目录既是一个个子模块, 更是一个模块名称空间, 即内部还有模块. 
在 Runtime 中导入一个模块直接 `add_subdirectory` 
对应的文件夹即可.

而内部的模块内部的小模块应该如何添加: 例如 Core 模块中添加 A 模块. 假设 A 的目录结构:
对外开放的头文件直接放在 A 的根目录下, 原因是这样在 Game 中可以 `#include "Core/A/A.h"`

使用 `add_internal_module` 函数, 来添加对应库信息
```CMake
# A/CMakeLists.txt:
# 主函数：添加内部模块
# 调用格式：
#   add_internal_module(<name>
#       TYPE      <STATIC|SHARED|OBJECT|INTERFACE>   # 可选，默认为 STATIC
#       SOURCES   <src1> [<src2> ...]                # 源文件列表
#       INCLUDES  <inc1> [<inc2> ...]                # 包含目录列表
#       DEPENDENCY <dep1> [<dep2> ...]               # 可选，依赖的其他目标
#       PRIVATE_INCLUDES  <inc1> [<inc2> ...]        # 私有包含目录列表, 例如 SDL3::SDL3
#   )
```
其中 `INCLUDES` 的参数是一个对外开放的头文件目录, 其应该是 `${CMAKE_CURRENT_SOURCE_DIR}`
对于内部头文件, 应该使用
```CMake
target_include_directories(MyModule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivateInclude
    ${CMAKE_CURRENT_SOURCE_DIR}/Source   # 如果私有头文件与源码混放
)
```
或者使用 `add_internal_module` 的 `PRIVATE_INCLUDES` 输入参数