cmake_minimum_required(VERSION 3.10)
project(Voxel-Forge)

# colmap
find_package(colmap REQUIRED)

#qt - gui
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_executable(Voxel-Forge src/main.cpp)
target_link_libraries(Voxel-Forge PRIVATE colmap::colmap Qt5::Core Qt5::Widgets)