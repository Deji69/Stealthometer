cmake_minimum_required(VERSION 3.15)

project(MyMod CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
set(ZHMMODSDK_VER "v2.0.0-rc.1")
include(cmake/setup-zhmmodsdk.cmake)

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

# Create the MyMod mod library.
add_library(MyMod SHARED
    src/MyMod.cpp
    src/MyMod.h
)

target_link_libraries(MyMod
    ZHMModSDK
)

install(TARGETS MyMod
    RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
zhmmodsdk_install(MyMod)
