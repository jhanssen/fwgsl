option(TINT_BUILD_SPV_READER "Build the SPIR-V input reader" ON)
option(TINT_BUILD_WGSL_READER "Build the WGSL input reader" ON)
option(TINT_BUILD_HLSL_WRITER "Build the HLSL output writer" OFF)
option(TINT_BUILD_MSL_WRITER "Build the MSL output writer" OFF)
option(TINT_BUILD_SPV_WRITER "Build the SPIR-V output writer" ON)
option(TINT_BUILD_WGSL_WRITER "Build the WGSL output writer" ON)
add_subdirectory(dawn-tint EXCLUDE_FROM_ALL)

