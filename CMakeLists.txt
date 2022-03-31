cmake_minimum_required(VERSION 3.14)

project(serial-latency LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#C++17 Required for functional-style thread creation
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core SerialPort)

set(CFILES "main.cpp")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_executable(${PROJECT_NAME}
        ${CFILES}
        icon.rc
    )
else()
    add_executable(${PROJECT_NAME}
      ${CFILES}
    )
endif()

target_link_libraries(serial-latency Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::SerialPort)
