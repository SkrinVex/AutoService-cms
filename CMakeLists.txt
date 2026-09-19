cmake_minimum_required(VERSION 3.16)
project(AutoService VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

add_executable(AutoService
    src/main.cpp
    src/Database.h
    src/Database.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/OrderDialog.h
    src/OrderDialog.cpp
)

target_include_directories(AutoService PRIVATE src)

set_target_properties(AutoService PROPERTIES
    WIN32_EXECUTABLE TRUE
    WIN23_EXECUTABLE TRUE
)

if(MSVC)
    target_compile_options(AutoService PRIVATE /W4 /utf-8)
else()
    target_compile_options(AutoService PRIVATE -Wall -Wextra)
endif()

target_link_libraries(AutoService PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)

enable_testing()
add_executable(AutoServiceTest
    test/test_main.cpp
    src/Database.h
    src/Database.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/OrderDialog.h
    src/OrderDialog.cpp
)
target_include_directories(AutoServiceTest PRIVATE src)
target_link_libraries(AutoServiceTest PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)
add_test(NAME AutoServiceTest COMMAND AutoServiceTest)
set_tests_properties(AutoServiceTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
