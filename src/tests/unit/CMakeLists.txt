
include_directories(../../test)

function(register_test test file)
    add_executable(${test} ${file})
    target_link_libraries(${test} core)
    platform_postprocess_executable(${test})
    # -u must appear before the archive in the link command (LINK_FLAGS precedes libraries).
    # Tests #include .cpp files directly, so --whole-archive would cause duplicate symbols.
    set_property(TARGET ${test} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-u,vector_table -Wl,-u,__dso_handle -Wl,-u,stbsp_vsnprintf")
    add_test(NAME ${test} COMMAND ${test})
endfunction(register_test)

add_subdirectory(core)
add_subdirectory(sequencer)
