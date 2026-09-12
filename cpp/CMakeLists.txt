add_library(starfield_game STATIC src/StarfieldGame.cpp)
target_include_directories(starfield_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(starfield_game PUBLIC cxx_std_17)
target_link_libraries(starfield_game PUBLIC cna_runtime)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(starfield_game PRIVATE -Wall -Wextra)
endif()

add_executable(starfield_cpp src/main.cpp)
target_link_libraries(starfield_cpp PRIVATE starfield_game)

add_executable(starfield_cpp_tests tests/StarfieldGameTests.cpp)
target_link_libraries(starfield_cpp_tests PRIVATE starfield_game)
add_test(NAME cpp_gameplay COMMAND starfield_cpp_tests)
set_tests_properties(
    cpp_gameplay
    PROPERTIES
        LABELS "cpp;gameplay;cna"
        ENVIRONMENT "SDL_VIDEODRIVER=offscreen")

add_test(
    NAME cpp_graphics_opengl33
    COMMAND starfield_cpp --smoke-frames 1 --validate-frame)
set_tests_properties(
    cpp_graphics_opengl33
    PROPERTIES
        LABELS "cpp;cna;graphics;opengl33"
        ENVIRONMENT "SDL_VIDEODRIVER=offscreen;CNA_GRAPHICS_RENDERER=OPENGL33"
        PASS_REGULAR_EXPRESSION
            "starfield-cpp state=playing collected=0 score=0 frame_valid=true")

if("OPENGLES3" IN_LIST CNA_GRAPHICS_RENDERERS OR
   CNA_GRAPHICS_RENDERER STREQUAL "OPENGLES3")
    add_test(
        NAME cpp_graphics_opengles3
        COMMAND starfield_cpp --smoke-frames 1 --validate-frame)
    set_tests_properties(
        cpp_graphics_opengles3
        PROPERTIES
            LABELS "cpp;cna;graphics;opengles3"
            ENVIRONMENT "SDL_VIDEODRIVER=offscreen;CNA_GRAPHICS_RENDERER=OPENGLES3"
            PASS_REGULAR_EXPRESSION
                "starfield-cpp state=playing collected=0 score=0 frame_valid=true")
endif()
