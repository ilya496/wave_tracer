add_executable(wave_tracer
    src/main.cpp
    src/EditorLayer.cpp
)

target_include_directories(wave_tracer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/glfw/include
)

target_link_libraries(wave_tracer PRIVATE wave_tracer_engine imgui implot OpenGL::GL)