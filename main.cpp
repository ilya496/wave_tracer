#include <iostream>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include "GLFW/glfw3.h"

int main() {
    drmp3 mp3;
    if (!drmp3_init_file(&mp3, "example.mp3", nullptr)) {
        // Handle error
    }
    drmp3_uint64 frameCount = drmp3_get_pcm_frame_count(&mp3);
    float* pSampleData = new float[frameCount * mp3.channels];
    drmp3_read_pcm_frames_f32(&mp3, frameCount, pSampleData);

    delete[] pSampleData;
    drmp3_uninit(&mp3);
    std::cout << "Hello, Wave Tracer!" << std::endl;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Wave Tracer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}