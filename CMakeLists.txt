# cmake_minimum_required(VERSION 3.10)
# project(Voxel-Forge LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # find Qt5
# find_package(Qt5 COMPONENTS Widgets REQUIRED)

# # enable automatic MOC, UIC, and RCC
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTORCC ON)


# add_executable(Voxel-Forge src/main.cpp)
# target_link_libraries(Voxel-Forge PRIVATE Qt5::Core Qt5::Widgets)


cmake_minimum_required(VERSION 3.18)
project(Voxel-Forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# enable automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-linux/include) # Needed because the vcpkg showed error of not being able to find OpenMVG include files due to may be some broken script in OpenMVG cmake files. So Manually linking the directory with there names as done below for OpenMVG alone.

# Don't search system paths first
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


# --- find all dependencies ---
# using vcpkg so no worries :)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# find_package(OpenMVG REQUIRED)
find_package(OpenMVS REQUIRED)

# Explicitly link to vcpkg libs
link_directories(${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-linux/lib)

add_executable(
    Voxel-Forge src/main.cpp
    src/openmvg_wrappers.cpp
    src/openmvg_wrappers.hpp
)

target_include_directories(Voxel-Forge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # so that #include "openmvg_wrappers.hpp" works
)

# --- link all libraries ---
target_link_libraries(Voxel-Forge PRIVATE 
    # Link Qt
    # note: Qt5::Widgets transitively links core and gui, 
    # so only need to specify the highest level component.
    Qt5::Widgets

    # link OpenMVG libs
    # link OpenMVG libs
    openMVG_sfm
    openMVG_features
    openMVG_matching
    openMVG_image
    openMVG_system
    openMVG_exif
    # openMVG_geodesy
    # openMVG_cameras
    openMVG_geometry
    openMVG_stlplus 
    openMVG_easyexif           


    # link OpenMVS libs
    # MVS - main library.
    OpenMVS::MVS
    OpenMVS::Common
    OpenMVS::IO
)