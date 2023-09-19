#include <iostream>
#include <glfw/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>
#include <vector>
#include "renderer.hpp"

// If using Dawn
#ifndef WEBGPU_BACKEND_DAWN
#define WEBGPU_BACKEND_DAWN
#endif

int main(int, char**)
{
    #pragma region Init GLFW
    if(!glfwInit())
    {
        std::cerr<< "Could not init GLFW!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // NEW
    GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    if (!window)
    {
        std::cerr<<"Could not create window!" << std::endl;
        glfwTerminate();
        return 1;
    }
    #pragma endregion

    Renderer renderer(window);

    #pragma region Main Loop
    while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		renderer.render(WGPUColor{ 0.9, 0.2, 0.2, 1.0 });
	}
    #pragma endregion
    
    #pragma region Cleanup
    renderer.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    #pragma endregion
    
    return 0;
}