add_executable(hnsw_benchmark2
    hnsw_benchmark2.cpp
    helper.cpp
    helper.h
)
target_include_directories(hnsw_benchmark2 PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(
    hnsw_benchmark2
    infinity_core
    sql_parser
    benchmark_profiler
)

add_executable(ann_ivfflat_benchmark
        ann_ivfflat_benchmark.cpp
        helper.cpp
        helper.h
)
target_include_directories(ann_ivfflat_benchmark PUBLIC "${CMAKE_SOURCE_DIR}/third_party/mlas")
target_include_directories(ann_ivfflat_benchmark PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(
        ann_ivfflat_benchmark
        infinity_core
        sql_parser
        benchmark_profiler
        onnxruntime_mlas
        iresearch
        atomic.a
        lz4.a
)
