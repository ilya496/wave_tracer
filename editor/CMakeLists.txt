add_executable(wave_tracer
    src/main.cpp
)

target_link_libraries(wave_tracer PRIVATE wave_tracer_engine)