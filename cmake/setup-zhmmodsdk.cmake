include(FetchContent)
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(
    ZHMModSDK
    URL https://github.com/OrfeasZ/ZHMModSDK/releases/download/${ZHMMODSDK_VER}/DevPkg-ZHMModSDK.zip
)

FetchContent_MakeAvailable(ZHMModSDK)
include("${zhmmodsdk_SOURCE_DIR}/cmake/zhm-mod.cmake")