cmake_minimum_required(VERSION 3.15)

project(Stealthometer CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
set(ZHMMODSDK_VER "v2.0.0-rc.1")
include(cmake/setup-zhmmodsdk.cmake)

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

# Create the Stealthometer mod library.
add_library(Stealthometer SHARED
	src/Stealthometer.cpp
	src/Stealthometer.h
 "src/Stats.h" "src/StatWindow.h" "src/StatWindow.cpp" "src/FixMinMax.h")

target_link_libraries(Stealthometer
	ZHMModSDK
)

install(TARGETS Stealthometer
	RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
zhmmodsdk_install(Stealthometer)
