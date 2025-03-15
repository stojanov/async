project(Async_Benchmarks)

add_executable(poll poll.cpp)

find_package(benchmark CONFIG REQUIRED)

target_link_libraries(poll ${benchmark_LIBRARIES})
