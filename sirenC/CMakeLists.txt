cmake_minimum_required(VERSION 3.14)

project(sirenC LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
# find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Threads REQUIRED)

add_executable(sirenC
  main.cpp
  client.h
  client.cpp
)
target_link_libraries(sirenC
  PRIVATE
    # Qt${QT_VERSION_MAJOR}::Core
    Threads::Threads
)
install(TARGETS sirenC
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
