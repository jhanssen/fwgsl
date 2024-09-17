cmake_minimum_required(VERSION 3.13)

project(
    Dawn
    DESCRIPTION "Dawn, a WebGPU implementation"
    LANGUAGES C CXX
    HOMEPAGE_URL "https://dawn.googlesource.com/dawn"
    VERSION 0.0.0
)

set(Dawn_SOURCE_DIR "${Dawn_SOURCE_DIR}/../dawn")

list(INSERT CMAKE_MODULE_PATH 0 "${Dawn_SOURCE_DIR}/src/cmake")

include(DawnCompilerChecks)
include(DawnCompilerExtraFlags)
include(DawnSetIfNotDefined)
include(DawnLibrary)

set_if_not_defined(DAWN_THIRD_PARTY_DIR "${Dawn_SOURCE_DIR}/third_party" "Directory in which to find third-party dependencies.")
set_if_not_defined(DAWN_SPIRV_TOOLS_DIR "${DAWN_THIRD_PARTY_DIR}/spirv-tools/src" "Directory in which to find SPIRV-Tools")
set_if_not_defined(DAWN_SPIRV_HEADERS_DIR "${DAWN_THIRD_PARTY_DIR}/spirv-headers/src" "Directory in which to find SPIRV-Headers")
set_if_not_defined(DAWN_GLSLANG_DIR "${DAWN_THIRD_PARTY_DIR}/glslang/src" "Directory in which to find GLSLang")
set_if_not_defined(DAWN_VULKAN_HEADERS_DIR "${DAWN_THIRD_PARTY_DIR}/vulkan-headers/src" "Directory in which to find Vulkan-Headers")
set_if_not_defined(DAWN_VULKAN_UTILITY_LIBRARIES_DIR "${DAWN_THIRD_PARTY_DIR}/vulkan-utility-libraries/src" "Directory in which to find Vulkan-Utility-Libraries")

set(DAWN_ENABLE_EMSCRIPTEN OFF)
set(TINT_LIB_FUZZING_ENGINE_LINK_OPTIONS "" CACHE STRING "Used by OSS-Fuzz to control, via link options, which fuzzing engine should be used")

set(TINT_ROOT_SOURCE_DIR   ${Dawn_SOURCE_DIR})
set(TINT_SPIRV_HEADERS_DIR ${DAWN_SPIRV_HEADERS_DIR})
set(TINT_SPIRV_TOOLS_DIR   ${DAWN_SPIRV_TOOLS_DIR})
add_subdirectory(../dawn/third_party ${CMAKE_CURRENT_BINARY_DIR}/dawn_third_party)
add_subdirectory(../dawn/src/utils ${CMAKE_CURRENT_BINARY_DIR}/dawn_utils)
add_subdirectory(../dawn/src/tint ${CMAKE_CURRENT_BINARY_DIR}/tint)
