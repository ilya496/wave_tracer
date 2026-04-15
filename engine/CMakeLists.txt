add_library(wave_tracer_engine STATIC
    src/core/Application.cpp
)

target_include_directories(wave_tracer_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_precompile_headers(wave_tracer_engine PUBLIC src/wtpch.h)