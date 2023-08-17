set(SOURCES 
    "main.cpp")

add_executable(fwgsl_embind ${SOURCES})
target_compile_options(fwgsl_embind PUBLIC -Oz)
set_target_properties(fwgsl_embind PROPERTIES SUFFIX ".mjs")

target_link_libraries(fwgsl_embind PRIVATE libtint)
target_compile_options(fwgsl_embind PRIVATE -std=c++17 -fno-exceptions -frtti)
target_link_options(fwgsl_embind PRIVATE -flto)
target_link_options(fwgsl_embind PRIVATE --bind -g4 -gsource-map -v -sASSERTIONS=1 -sSAFE_HEAP=1 -sTOTAL_STACK=8388608 -sWASM=1 -sMODULARIZE=1 -sSINGLE_FILE=1 -sDEMANGLE_SUPPORT=1 -sALLOW_MEMORY_GROWTH=1 -sDISABLE_EXCEPTION_CATCHING=1 -sEXPORT_NAME=fwgsl -sNODEJS_CATCH_EXIT=0 -sNODEJS_CATCH_REJECTION=0 "-sEXPORTED_RUNTIME_METHODS=[\"craftInvokerFunction\"]")
set_target_properties(fwgsl_embind PROPERTIES OUTPUT_NAME "fwgsl_embind")
