set(SOURCES 
    "Source/main.cpp")

add_executable(fwgsl ${SOURCES})
target_compile_options(fwgsl PRIVATE -Oz)
target_compile_options(fwgsl PUBLIC "SHELL:-s ALLOW_MEMORY_GROWTH=1")
target_link_libraries(fwgsl PRIVATE libtint)
target_link_options(fwgsl PRIVATE -s EXPORTED_FUNCTIONS=["_wgsl_to_spirv","_spirv_to_wgsl","_free","_malloc"])
target_link_options(fwgsl PRIVATE -s --js-library ${CMAKE_CURRENT_SOURCE_DIR}/Source/callbacks.js)
target_link_options(fwgsl PRIVATE --extern-pre-js ${CMAKE_CURRENT_SOURCE_DIR}/head.js)
target_link_options(fwgsl PRIVATE --extern-post-js ${CMAKE_CURRENT_SOURCE_DIR}/tail.js)
target_link_options(fwgsl PUBLIC "SHELL:-s ALLOW_MEMORY_GROWTH=1")
target_link_options(fwgsl PRIVATE -Oz)
target_link_options(fwgsl PRIVATE -flto)
