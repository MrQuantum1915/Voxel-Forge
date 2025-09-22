cmake_minimum_required(VERSION 3.10)
project(Voxel-Forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find Qt5
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# enable automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


add_executable(Voxel-Forge src/main.cpp)
target_link_libraries(Voxel-Forge PRIVATE Qt5::Core Qt5::Widgets)