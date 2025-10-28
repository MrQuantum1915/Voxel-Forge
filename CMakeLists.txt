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

# --- find all dependencies ---
# vcpkg will automatically find these packages when using the toolchain.
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(OpenMVG REQUIRED)
find_package(OpenMVS REQUIRED)



add_executable(Voxel-Forge src/main.cpp)

# --- link all libraries ---
target_link_libraries(Voxel-Forge PRIVATE 
    # Link Qt
    # note: Qt5::Widgets transitively links core and gui, 
    # so only need to specify the highest level component.
    Qt5::Widgets

    # link OpenMVG libs
    OpenMVG::openMVG_sfm
    OpenMVG::openMVG_features
    OpenMVG::openMVG_matching
    OpenMVG::openMVG_image

    # link OpenMVS libs
    # MVS - main library.
    OpenMVS::MVS
    OpenMVS::Common
    OpenMVS::IO
)