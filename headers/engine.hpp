#ifndef ENGINE
#define ENGINE
#include <memory>
#include "renderer.hpp"


struct DestroyglfwWin{

    void operator()(GLFWwindow* ptr){
         glfwDestroyWindow(ptr);
    }

};

typedef std::unique_ptr<GLFWwindow, DestroyglfwWin> smart_GLFWwindow;

class Engine
{
public:
    Engine();
    ~Engine();
    void start();
private:
    std::unique_ptr<Renderer> renderer;
    GLFWwindow *window;
};
#endif