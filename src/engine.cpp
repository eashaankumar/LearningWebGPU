#include <iostream>
#include <glfw/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>
#include <vector>

#include "engine.hpp"

// If using Dawn
#ifndef WEBGPU_BACKEND_DAWN
#define WEBGPU_BACKEND_DAWN
#endif

Engine::Engine()
{

    #pragma region Init GLFW
    if(!glfwInit())
    {
        std::cerr<< "Could not init GLFW!" << std::endl;
        throw std::exception();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // NEW
    window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    if (!window)
    {
        std::cerr<<"Could not create window!" << std::endl;
        glfwTerminate();
        throw std::exception();
    }
    #pragma endregion

    Renderer rend(window);
    renderer = &rend;

    while (!glfwWindowShouldClose(window)) {
        // Do nothing, this checks for ongoing asynchronous operations and call their callbacks
        #ifdef WEBGPU_BACKEND_WGPU
            // Non-standardized behavior: submit empty queue to flush callbacks
            // (wgpu-native also has a wgpuDevicePoll but its API is more complex)
            wgpuQueueSubmit(queue, 0, nullptr);
        #else
            // Non-standard Dawn way
            WGPUDevice d = *(renderer->device);
            wgpuDeviceTick( d );
        #endif
        std::cout << "WORKS" << std::endl;
	    glfwPollEvents();
    	renderer->render(WGPUColor{ 0.9, 0.2, 0.2, 1.0 });	
	}
    glfwDestroyWindow(window);
    glfwTerminate();
}

