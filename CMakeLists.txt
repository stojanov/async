cmake_minimum_required(VERSION 3.28)

# set(VCPKG_OVERLAY_TRIPLETS "triplets") set(VCPKG_TARGET_TRIPLET "x64-linux")

project(async)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

# add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF)
find_package(GTest CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ENABLE_TESTING ON)
set(ENABLE_ASAN ON)

if (${ENABLE_ASAN})

endif()

# add_compile_options(-fsanitize=address -fno-sanitize=alignment -fsanitize=enum
#                     -fsanitize=leak)
#
# link_libraries(-fsanitize=address -fno-sanitize=alignment -fsanitize=enum
#                -fsanitize=leak)

link_libraries(spdlog::spdlog)

# set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")

set(LIBRARY_NAME "${PROJECT_NAME}")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/async")

add_library(${LIBRARY_NAME})

target_sources(${LIBRARY_NAME}
    PRIVATE 
        ${LIB_DIR}/defines.h
        ${LIB_DIR}/utils.h
        ${LIB_DIR}/pch.h

        ${LIB_DIR}/runtime/coroutine.h
        ${LIB_DIR}/runtime/coroutine.cpp
        ${LIB_DIR}/runtime/defines.h
        ${LIB_DIR}/runtime/io_context_thread.cpp
        ${LIB_DIR}/runtime/io_context_thread.h
        ${LIB_DIR}/runtime/io_thread_handler.cpp
        ${LIB_DIR}/runtime/io_thread_handler.h
        ${LIB_DIR}/runtime/runqueue.cpp
        ${LIB_DIR}/runtime/runqueue.h
        ${LIB_DIR}/runtime/runtime.cpp
        ${LIB_DIR}/runtime/runtime.h
        ${LIB_DIR}/runtime/runtime_core.cpp
        ${LIB_DIR}/runtime/runtime_core.h
        ${LIB_DIR}/runtime/timer.cpp
        ${LIB_DIR}/runtime/timer.h
        ${LIB_DIR}/runtime/timer_thread.cpp
        ${LIB_DIR}/runtime/timer_thread.h
        ${LIB_DIR}/runtime/prio_timer_thread.cpp
        ${LIB_DIR}/runtime/prio_timer_thread.h
        ${LIB_DIR}/runtime/timer_thread_handler.cpp
        ${LIB_DIR}/runtime/timer_thread_handler.h
        ${LIB_DIR}/runtime/worker_thread.cpp
        ${LIB_DIR}/runtime/worker_thread.h
        
        ${LIB_DIR}/io/pal/op/read.h
        ${LIB_DIR}/io/pal/io_context.h
        ${LIB_DIR}/io/pal/io_handle.h
        ${LIB_DIR}/io/pal/io_op.h
        ${LIB_DIR}/io/pal/io_type.h

        ${LIB_DIR}/io/platform/linux/io_context.cpp
        ${LIB_DIR}/io/platform/linux/io_context.h
        ${LIB_DIR}/io/platform/linux/io_handle.h
        ${LIB_DIR}/io/platform/linux/io_op.h
        ${LIB_DIR}/io/platform/linux/op/read.h

        ${LIB_DIR}/io/base.h

    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src FILES
        ${LIB_DIR}/wait_group.h
        ${LIB_DIR}/sleep.h
        ${LIB_DIR}/select.h
        ${LIB_DIR}/read.h
        ${LIB_DIR}/poll.h
        ${LIB_DIR}/mutex.h
        ${LIB_DIR}/select.h
)

target_precompile_headers(${LIBRARY_NAME} PUBLIC src/async/pch.h)
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)

if (WIN32)
    target_compile_definitions(${LIBRARY_NAME} PUBLIC PLATFORM_WINDOWS)
elseif (APPLE)
    target_compile_definitions(${LIBRARY_NAME} PUBLIC PLATFORM_MACOS)
elseif (UNIX)
    target_compile_definitions(${LIBRARY_NAME} PUBLIC PLATFORM_LINUX)
endif()

install(TARGETS ${LIBRARY_NAME} FILE_SET HEADERS)


if (${ENABLE_TESTING})
    enable_testing()

    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

    set(TEST_NAME "${PROJECT_NAME}_tests")
    add_executable(${TEST_NAME} ${TEST_SOURCES})

# target_precompile_headers(${TEST_NAME} PUBLIC src/pch.h)

    target_include_directories(${TEST_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
    target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest GTest::gtest_main
                                               GTest::gmock GTest::gmock_main)

    target_link_libraries(${TEST_NAME} PRIVATE ${LIBRARY_NAME} spdlog::spdlog)

    target_compile_definitions(${TEST_NAME}
                               PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF)
    include(GoogleTest)
    gtest_discover_tests(${TEST_NAME})

# add_subdirectory(${CMAKE_SOURCE_DIR}/benchmarks/)

endif()

