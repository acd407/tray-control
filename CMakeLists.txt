cmake_minimum_required(VERSION 3.23)
project(tray_control
    VERSION 0.0.1
    DESCRIPTION "Simple CLI tool to show items in systray and activate them"
    LANGUAGES CXX
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 生成编译命令数据库
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
else()
    add_compile_options(-O3)
endif()

# 添加编译器特定选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 使用 FetchContent 管理第三方依赖
include(FetchContent)

# 设置FetchContent的下载目录
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)

# cxxopts - 命令行选项解析库
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.1.1
  GIT_SHALLOW    1
)
FetchContent_MakeAvailable(cxxopts)

# magic_enum - C++枚举反射库
FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.9.3
  GIT_SHALLOW    1
)
FetchContent_MakeAvailable(magic_enum)

# ftxui - 终端UI库
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui.git
  GIT_TAG        v5.0.0
  GIT_SHALLOW    1
)
FetchContent_MakeAvailable(ftxui)

# 查找系统依赖
find_package(sdbus-c++ REQUIRED)
find_package(fmt REQUIRED)

# 添加源文件目录
include_directories(src)

# 创建核心库
add_library(core STATIC
    src/StatusNotifierWatcher.cpp
    src/StatusNotifierItem.cpp
    src/DBusMenu.cpp
)

# 设置核心库的属性
target_include_directories(core PUBLIC src)
target_link_libraries(core 
    PUBLIC 
        SDBusCpp::sdbus-c++
        magic_enum
)

# 创建可执行文件
function(add_tray_executable name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} core cxxopts fmt)
    
    # 设置可执行文件的属性
    set_target_properties(${name} PROPERTIES
        OUTPUT_NAME ${name}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # 安装可执行文件
    install(TARGETS ${name}
        RUNTIME DESTINATION bin
    )
endfunction()

# 添加各个工具
add_tray_executable(tray-show src/tray-show.cpp)
add_tray_executable(tray-activate src/tray-activate.cpp)
add_tray_executable(tray-trigger src/tray-trigger.cpp)

# 特殊处理tray-navigate，因为它需要额外的ftxui库
add_executable(tray-navigate src/tray-navigate.cpp)
target_link_libraries(tray-navigate core cxxopts fmt ftxui::screen ftxui::dom ftxui::component)
set_target_properties(tray-navigate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
install(TARGETS tray-navigate
    RUNTIME DESTINATION bin
)

# 添加自定义目标用于清理
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/_deps
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMENT "Cleaning all build files and dependencies"
)

# 打印配置信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")