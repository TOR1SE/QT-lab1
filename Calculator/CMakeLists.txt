cmake_minimum_required(VERSION 3.16)

project(Calculator VERSION 1.0 LANGUAGES CXX)

# Qt 元对象系统：自动处理 .ui / Q_OBJECT / .qrc
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(Calculator
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
)

target_link_libraries(Calculator PRIVATE Qt6::Widgets)

# ---------------------------------------------------------------------------
# 构建路径含中文等「非 ASCII 字符」的兼容处理
# 现象：moc 无法在中文构建目录下创建 Calculator_autogen/EWIEGA46WW/moc_xxx.cpp，
#       报 "Cannot create ... No such file or directory"，随后编译中断。
# 对策：把 AUTOGEN_BUILD_DIR（moc/uic 产物目录）重定向到系统临时目录，
#       保证无论工程放在什么路径下都能正常编译。
# ---------------------------------------------------------------------------
set(_ascii_pattern "[^A-Za-z0-9:/_. +-\\\\]")   # 出现白名单之外的字符即视为含中文等非 ASCII 字符
set(_autogen_dir "${CMAKE_CURRENT_BINARY_DIR}/Calculator_autogen")
string(REGEX MATCH "${_ascii_pattern}" _non_ascii "${CMAKE_CURRENT_BINARY_DIR}")
if(_non_ascii)
    # 优先使用系统临时目录；若临时目录自身也含中文，则退回系统级纯 ASCII 目录
    set(_fallback_base "$ENV{TEMP}")
    string(REGEX MATCH "${_ascii_pattern}" _tmp_non_ascii "${_fallback_base}")
    if(_tmp_non_ascii OR _fallback_base STREQUAL "")
        if(WIN32)
            set(_fallback_base "C:/Windows/Temp")
        else()
            set(_fallback_base "/tmp")
        endif()
    endif()
    string(MD5 _dir_hash "${CMAKE_CURRENT_BINARY_DIR}")
    set(_autogen_dir "${_fallback_base}/Calculator_autogen_${_dir_hash}")
    file(MAKE_DIRECTORY "${_autogen_dir}")
    message(STATUS "构建路径含非 ASCII 字符，moc/uic 产物目录已重定向到: ${_autogen_dir}")
endif()
set_target_properties(Calculator PROPERTIES AUTOGEN_BUILD_DIR "${_autogen_dir}")

# ---------------------------------------------------------------------------
# 自测程序：以程序方式模拟点击与按键，对全部计算路径做回归验证
# 退出码 0 表示所有用例通过，用于回归测试与边界场景验证
# ---------------------------------------------------------------------------
option(BUILD_SELF_CHECK "构建计算器自测程序" ON)
if(BUILD_SELF_CHECK)
    add_executable(CalculatorSelfCheck
        tests/SelfCheck/main.cpp
        mainwindow.h
        mainwindow.cpp
        mainwindow.ui
    )
    target_include_directories(CalculatorSelfCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(CalculatorSelfCheck PRIVATE Qt6::Widgets)
    if(MSVC)
        target_compile_options(CalculatorSelfCheck PRIVATE /utf-8)
    else()
        target_compile_options(CalculatorSelfCheck PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
        )
    endif()
    set_target_properties(CalculatorSelfCheck PROPERTIES AUTOGEN_BUILD_DIR "${_autogen_dir}_selfcheck")
endif()

# ---------------------------------------------------------------------------
# 演示模式：按脚本自动驱动计算器（点击 + 键盘事件），配合录屏生成测试视频
# ---------------------------------------------------------------------------
option(BUILD_DEMO "构建演示模式程序" ON)
if(BUILD_DEMO)
    add_executable(CalculatorDemo
        tools/Demo/main.cpp
        mainwindow.h
        mainwindow.cpp
        mainwindow.ui
    )
    target_include_directories(CalculatorDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(CalculatorDemo PRIVATE Qt6::Widgets)
    if(MSVC)
        target_compile_options(CalculatorDemo PRIVATE /utf-8)
    else()
        target_compile_options(CalculatorDemo PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
        )
    endif()
    set_target_properties(CalculatorDemo PROPERTIES AUTOGEN_BUILD_DIR "${_autogen_dir}_demo")
endif()

# 保证源码与 QSS 中的中文在 MinGW/MSVC 下均不出现乱码
if(MSVC)
    target_compile_options(Calculator PRIVATE /utf-8)
else()
    target_compile_options(Calculator PRIVATE
        -finput-charset=UTF-8
        -fexec-charset=UTF-8
    )
endif()
