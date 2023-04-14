# If ZHMMODSDK_DIR is set, use that. Otherwise, download the SDK from GitHub.
if (DEFINED ZHMMODSDK_DIR)
    include("${ZHMMODSDK_DIR}/cmake/sdk-local.cmake")
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