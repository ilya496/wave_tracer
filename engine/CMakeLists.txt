add_library(wave_tracer_engine STATIC
    src/core/Application.cpp
    src/core/Window.cpp
    src/core/Layer.cpp
    src/core/RenderLayer.cpp
    src/import/AudioImporter.cpp
)

target_include_directories(wave_tracer_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui/backends
    ${CMAKE_SOURCE_DIR}/libs/glm
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(wave_tracer_engine PUBLIC glfw OpenGL::GL glm glad)

target_precompile_headers(wave_tracer_engine PUBLIC src/wtpch.h)