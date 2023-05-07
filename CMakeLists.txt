cmake_minimum_required(VERSION 3.15)

project(Stealthometer CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
set(ZHMMODSDK_VER "v2.0.0-rc.1")
include(cmake/setup-zhmmodsdk.cmake)

# Download CMakeRC resource compiler
file(DOWNLOAD "https://raw.githubusercontent.com/vector-of-bool/cmrc/master/CMakeRC.cmake"
                 "${CMAKE_BINARY_DIR}/CMakeRC.cmake")
include("${CMAKE_BINARY_DIR}/CMakeRC.cmake")

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

#create_resources("data" "src/resources.c")
cmrc_add_resource_library(stealthometer-resources NAMESPACE stealthometer ALIAS stealthometer::rc "data/repo.json")

# Create the Stealthometer mod library.
add_library(Stealthometer SHARED
	"src/Stealthometer.cpp"
	"src/Stealthometer.h"
 "src/Stats.h" "src/StatWindow.h" "src/StatWindow.cpp" "src/FixMinMax.h"
 "src/Rating.h" "src/Rating.cpp" "src/PlayStyleRating.h" "src/util.h" "src/Events.h" "src/EventSystem.h" "src/EventSystem.cpp" "src/Enums.h")

target_link_libraries(Stealthometer PRIVATE
	ZHMModSDK
	stealthometer::rc
)

install(TARGETS Stealthometer
	RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
zhmmodsdk_install(Stealthometer)
