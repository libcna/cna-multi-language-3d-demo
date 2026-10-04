add_executable(starfield_c StarfieldGame.c)
target_compile_features(starfield_c PRIVATE c_std_99)
target_link_libraries(starfield_c PRIVATE cna_c_api m)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(starfield_c PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_target(
    starfield_c_content
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/Content"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../cpp/content" "${CMAKE_CURRENT_BINARY_DIR}/Content"
    COMMENT "Copying Starfield Courier C content")
add_dependencies(starfield_c starfield_c_content)

add_executable(starfield_c_tests StarfieldGame.c)
target_compile_features(starfield_c_tests PRIVATE c_std_99)
target_compile_definitions(starfield_c_tests PRIVATE STARFIELD_GAMEPLAY_TEST=1)
target_link_libraries(starfield_c_tests PRIVATE cna_c_api m)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(starfield_c_tests PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-function)
endif()

add_test(NAME c_gameplay COMMAND starfield_c_tests)
set_tests_properties(c_gameplay PROPERTIES LABELS "c;gameplay;cna")
foreach(sector IN ITEMS 1 2 3)
    add_test(NAME c_graphics_sector${sector}
        COMMAND starfield_c --smoke-frames 1 --validate-frame --start-sector ${sector})
    set_tests_properties(c_graphics_sector${sector} PROPERTIES
        LABELS "c;cna;graphics;sectors"
        ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "starfield-c state=playing collected=0 score=0 sector=${sector} frame_valid=true")
endforeach()
