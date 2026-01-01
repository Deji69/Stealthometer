# If ZHMMODSDK_DIR is set, use that. Otherwise, download the SDK from GitHub.
if (DEFINED ZHMMODSDK_DIR)
    add_library(ZHMModSDK SHARED IMPORTED GLOBAL)

    set(ZHM_BUILD_TYPE "release")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(ZHM_BUILD_TYPE "debug")
    endif()

    # If ZHMMODSDK_DIST_DIR is not defined, load the sdk from the current source directory.
    if(NOT DEFINED ZHMMODSDK_DIST_DIR)
        set(ZHMMODSDK_DIST_DIR "${ZHMMODSDK_DIR}/${ZHM_BUILD_TYPE}")
    endif()

    if(NOT DEFINED ZHMMODSDK_INCLUDE_DIR)
        set(ZHMMODSDK_INCLUDE_DIR "${ZHMMODSDK_DIR}/include")
    endif()

    set_target_properties(ZHMModSDK PROPERTIES
        IMPORTED_LOCATION "${ZHMMODSDK_DIST_DIR}/bin/ZHMModSDK.dll"
        IMPORTED_IMPLIB "${ZHMMODSDK_DIST_DIR}/lib/ZHMModSDK.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${ZHMMODSDK_INCLUDE_DIR}"
        INTERFACE_LINK_DIRECTORIES "${ZHMMODSDK_DIST_DIR}/lib"
        INTERFACE_COMPILE_DEFINITIONS "SPDLOG_FMT_EXTERNAL"
    )

    target_link_libraries(ZHMModSDK INTERFACE
        fmt
        imgui
    )

else()
    include(FetchContent)
    cmake_policy(SET CMP0135 NEW)
    FetchContent_Declare(
        ZHMModSDK
        URL https://github.com/OrfeasZ/ZHMModSDK/releases/download/${ZHMMODSDK_VER}/DevPkg-ZHMModSDK.zip
    )

    FetchContent_MakeAvailable(ZHMModSDK)
    set(ZHMMODSDK_DIR "${zhmmodsdk_SOURCE_DIR}")
endif()

include("${ZHMMODSDK_DIR}/cmake/zhm-mod.cmake")