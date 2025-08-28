cmake_minimum_required(VERSION 3.10)
project(Voxel_Forge)

# colmap
find_package(colmap REQUIRED)

#qt - gui
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_executable(Voxel_Forge src/main.cpp)
target_link_libraries(Voxel_Forge PRIVATE colmap::colmap Qt5::Core Qt5::Widgets)