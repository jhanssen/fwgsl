set(SOURCES 
    "Source/main.cpp")

add_executable(fwgsl ${SOURCES})
target_compile_options(fwgsl PRIVATE -Oz)
set_target_properties(fwgsl PROPERTIES SUFFIX ".mjs")
target_link_libraries(fwgsl PRIVATE libtint)
target_link_options(fwgsl PRIVATE -s EXPORTED_FUNCTIONS=["_wgsl_to_spirv","_spirv_to_wgsl","_free","_malloc"])
target_link_options(fwgsl PRIVATE -s --js-library ${CMAKE_CURRENT_SOURCE_DIR}/Source/callbacks.js)
target_link_options(fwgsl PUBLIC "SHELL:-s ALLOW_MEMORY_GROWTH=1 -s EXPORT_ES6 -sSINGLE_FILE")
target_link_options(fwgsl PRIVATE -Oz)
target_link_options(fwgsl PRIVATE -flto)
