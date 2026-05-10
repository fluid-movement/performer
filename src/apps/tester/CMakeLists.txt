set(sources
    Tester.cpp
)

include_directories(.)

if(${PLATFORM} STREQUAL "stm32")
    add_executable(tester ${sources})
    target_link_libraries(tester "-Wl,--whole-archive" core "-Wl,--no-whole-archive" opencm3_stm32f4)
    platform_postprocess_executable(tester)
    # override linker script
    set_target_properties(tester PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/tester.ld")

    add_executable(tester_standalone ${sources})
    target_link_libraries(tester_standalone "-Wl,--whole-archive" core "-Wl,--no-whole-archive" opencm3_stm32f4)
    platform_postprocess_executable(tester_standalone)
    # override linker script
    set_target_properties(tester_standalone PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/tester_standalone.ld")
endif()
