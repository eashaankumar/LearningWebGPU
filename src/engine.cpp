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
    renderer = std::make_unique<Renderer>(new Renderer((GLFWwindow *)window));
}

Engine::~Engine()
{
    renderer.reset();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::start()
{
    while (!glfwWindowShouldClose(window)) {
	    glfwPollEvents();
    	renderer->render(WGPUColor{ 0.9, 0.2, 0.2, 1.0 });	
	}
}