add_library(rpp_tests_utils INTERFACE)
target_include_directories(rpp_tests_utils INTERFACE .)
target_link_libraries(rpp_tests_utils INTERFACE rpp)

target_compile_options(rpp_tests_utils INTERFACE "-w")
set_target_properties(rpp_tests_utils PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(rpp_tests_utils PROPERTIES CXX_CPPCHECK "")
