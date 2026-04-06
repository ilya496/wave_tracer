add_library(wave_tracer_engine STATIC
    src/core/Application.cpp
    # src/core/Window.cpp
    # src/core/LayerStack.cpp
    # src/core/UUID.cpp
    # src/audio/AudioEngine.cpp
    # src/audio/AudioMixer.cpp
)

target_precompile_headers(wave_tracer_engine PUBLIC src/wtpch.h)

