add_library(starfield_core STATIC src/starfield.cpp)
target_include_directories(starfield_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(starfield_core PUBLIC cxx_std_17)

add_executable(starfield_cpp src/main.cpp)
target_link_libraries(starfield_cpp PRIVATE starfield_core)

if(STARFIELD_ENABLE_CNA)
    add_executable(starfield_cna src/cna_game.cpp)
    target_link_libraries(starfield_cna PRIVATE starfield_core cna_runtime)
endif()

enable_testing()
add_executable(starfield_cpp_tests tests/starfield_tests.cpp)
target_link_libraries(starfield_cpp_tests PRIVATE starfield_core)
add_test(NAME cpp_gameplay COMMAND starfield_cpp_tests)
set_tests_properties(cpp_gameplay PROPERTIES LABELS "cpp;gameplay")

if(STARFIELD_ENABLE_CNA)
    foreach(scenario IN ITEMS collection hazard win restart)
        add_test(
            NAME cpp_cna_${scenario}
            COMMAND starfield_cna --scenario ${scenario} --frames 1 --validate-frame)
        set_tests_properties(
            cpp_cna_${scenario}
            PROPERTIES
                LABELS "cpp;cna;graphics"
                ENVIRONMENT "SDL_VIDEODRIVER=offscreen")
    endforeach()
    set_tests_properties(cpp_cna_collection PROPERTIES
        PASS_REGULAR_EXPRESSION "cna-cpp state=1 collected=7 score=300")
    set_tests_properties(cpp_cna_hazard PROPERTIES
        PASS_REGULAR_EXPRESSION "cna-cpp state=3 collected=0 score=0")
    set_tests_properties(cpp_cna_win PROPERTIES
        PASS_REGULAR_EXPRESSION "cna-cpp state=2 collected=7 score=1300")
    set_tests_properties(cpp_cna_restart PROPERTIES
        PASS_REGULAR_EXPRESSION "cna-cpp state=0 collected=0 score=0")

    if("OPENGLES3" IN_LIST CNA_GRAPHICS_RENDERERS OR
       CNA_GRAPHICS_RENDERER STREQUAL "OPENGLES3")
        foreach(scenario IN ITEMS collection hazard win restart)
            add_test(
                NAME cpp_cna_opengles3_${scenario}
                COMMAND starfield_cna --scenario ${scenario} --frames 1 --validate-frame)
            set_tests_properties(
                cpp_cna_opengles3_${scenario}
                PROPERTIES
                    LABELS "cpp;cna;graphics;opengles"
                    ENVIRONMENT "SDL_VIDEODRIVER=offscreen;CNA_GRAPHICS_RENDERER=OPENGLES3")
        endforeach()
        set_tests_properties(cpp_cna_opengles3_collection PROPERTIES
            PASS_REGULAR_EXPRESSION
                "graphics renderer: OPENGLES3.*cna-cpp state=1 collected=7 score=300")
        set_tests_properties(cpp_cna_opengles3_hazard PROPERTIES
            PASS_REGULAR_EXPRESSION
                "graphics renderer: OPENGLES3.*cna-cpp state=3 collected=0 score=0")
        set_tests_properties(cpp_cna_opengles3_win PROPERTIES
            PASS_REGULAR_EXPRESSION
                "graphics renderer: OPENGLES3.*cna-cpp state=2 collected=7 score=1300")
        set_tests_properties(cpp_cna_opengles3_restart PROPERTIES
            PASS_REGULAR_EXPRESSION
                "graphics renderer: OPENGLES3.*cna-cpp state=0 collected=0 score=0")
    endif()
endif()
