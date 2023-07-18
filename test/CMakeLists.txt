# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(raccoonTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(GTest REQUIRED)
include(GoogleTest)

# ---- Tests ----

add_executable(raccoon_test src/raccoon_test.cpp)
target_link_libraries(
    raccoon_test PRIVATE
    raccoon_lib
    GTest::gtest_main
)
target_compile_features(raccoon_test PRIVATE cxx_std_20)

gtest_discover_tests(raccoon_test)

# ---- End-of-file commands ----

add_folders(Test)
