cmake_minimum_required(VERSION 3.15)

project(Stealthometer CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(ZHMMODSDK_VER "v3.1.1")
include(cmake/setup-zhmmodsdk.cmake)

# Download CMakeRC resource compiler
file(DOWNLOAD "https://raw.githubusercontent.com/vector-of-bool/cmrc/master/CMakeRC.cmake"
                 "${CMAKE_BINARY_DIR}/CMakeRC.cmake")
include("${CMAKE_BINARY_DIR}/CMakeRC.cmake")

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

#create_resources("data" "src/resources.c")
cmrc_add_resource_library(stealthometer-resources NAMESPACE stealthometer ALIAS stealthometer::rc "data/repo.json" "data/npc.json")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Create the Stealthometer mod library.
add_library(Stealthometer SHARED
 "src/Stealthometer.cpp"
 "src/Stealthometer.h"
 "src/Stats.h" "src/StatWindow.h" "src/StatWindow.cpp" "src/FixMinMax.h"
 "src/Rating.h" "src/Rating.cpp" "src/PlayStyleRating.h" "src/util.h" "src/Events.h" "src/EventSystem.h" "src/EventSystem.cpp" "src/Enums.h"
 "src/deps/imgui/imgui_stdlib.h"
 "src/deps/imgui/imgui_stdlib.cpp"
 "src/LiveSplitClient.h" "src/LiveSplitClient.cpp" "src/RunData.h")


find_package(directx-headers CONFIG REQUIRED)

target_link_libraries(Stealthometer PRIVATE
	ZHMModSDK
	ImGui
	stealthometer::rc
	Microsoft::DirectX-Guids
	Microsoft::DirectX-Headers
)

install(TARGETS Stealthometer
	RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
zhmmodsdk_install(Stealthometer)
