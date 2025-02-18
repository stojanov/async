cmake_minimum_required(VERSION 3.28)

set(VCPKG_OVERLAY_TRIPLETS "triplets")
set(VCPKG_TARGET_TRIPLET "x64-linux")

project(async)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE ASYNC_SOURCES src/*.cpp)
file(GLOB_RECURSE ASYNC_HEADERS src/*.h)

add_executable(${PROJECT_NAME} ${ASYNC_SOURCES})

add_compile_options(-fsanitize=address -fno-sanitize=alignment -fsanitize=enum
                    -fsanitize=leak)

link_libraries(-fsanitize=address -fno-sanitize=alignment -fsanitize=enum
               -fsanitize=leak)

target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
# set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

set(LIBRARY_NAME "${PROJECT_NAME}_lib")

add_library(${LIBRARY_NAME} STATIC ${ASYNC_SOURCES})
target_precompile_headers(${LIBRARY_NAME} PUBLIC src/pch.h)
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)

find_package(GTest CONFIG REQUIRED)

enable_testing()

file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

set(TEST_NAME "${PROJECT_NAME}_tests")
add_executable(${TEST_NAME} ${TEST_SOURCES})

# target_precompile_headers(${TEST_NAME} PUBLIC src/pch.h)

target_include_directories(${TEST_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest GTest::gtest_main
                                           GTest::gmock GTest::gmock_main)

target_link_libraries(${TEST_NAME} PRIVATE ${LIBRARY_NAME})

include(GoogleTest)
gtest_discover_tests(${TEST_NAME})
