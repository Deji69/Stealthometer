cmake_minimum_required(VERSION 3.15)

project(Stealthometer CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
set(ZHMMODSDK_VER "v2.0.0-rc.1")
include(cmake/setup-zhmmodsdk.cmake)

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

# Embed resources
function(create_resources dir output)
	# Create empty output file
	file(WRITE ${output} "")
	# Collect input files
	file(GLOB bins ${dir}/*)
	# Iterate through input files
	foreach(bin ${bins})
		# Get short filename
		string(REGEX MATCH "([^/]+)$" filename ${bin})
		# Replace filename spaces & extension separator for C compatibility
		string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
		# Read hex data from file
		file(READ ${bin} filedata HEX)
		# Convert hex data for C compatibility
		string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
		# Append data to output file
		file(APPEND ${output} "// Auto-generated file, do not modify.\n\nconst unsigned char ${filename}[] = {${filedata}};\nconst size_t ${filename}_size = sizeof(${filename});\n")
	endforeach()
endfunction()

create_resources("data" "src/resources.c")

# Create the Stealthometer mod library.
add_library(Stealthometer SHARED
	"src/resources.c"
	"src/Stealthometer.cpp"
	"src/Stealthometer.h"
 "src/Stats.h" "src/StatWindow.h" "src/StatWindow.cpp" "src/FixMinMax.h"
 "src/Resources.h")

target_link_libraries(Stealthometer
	ZHMModSDK
)

install(TARGETS Stealthometer
	RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
zhmmodsdk_install(Stealthometer)
