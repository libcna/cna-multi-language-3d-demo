enable_testing()

if(NOT STARFIELD_ENABLE_CNA)
    message(STATUS "Skipping C demo: configure with STARFIELD_ENABLE_CNA=ON to use CNA's C API")
    return()
endif()

add_executable(cna_starfield_c cna_starfield.c)
target_link_libraries(cna_starfield_c PRIVATE cna_c_api)
target_include_directories(cna_starfield_c PRIVATE "${CNA_SOURCE_DIR}/modules/c-api/include")

add_executable(cna_c_api_consumer_smoke cna_c_api_consumer_smoke.c)
target_link_libraries(cna_c_api_consumer_smoke PRIVATE cna_c_api)
target_include_directories(cna_c_api_consumer_smoke PRIVATE
    "${CNA_SOURCE_DIR}/modules/c-api/include")
add_test(NAME cna_c_api_consumer_smoke COMMAND cna_c_api_consumer_smoke)