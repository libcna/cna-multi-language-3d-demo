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
add_test(NAME cpp_startup COMMAND starfield_cpp startup)
add_test(NAME cpp_collection COMMAND starfield_cpp collection)