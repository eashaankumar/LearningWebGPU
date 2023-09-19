#ifndef ENGINE
#define ENGINE
#include <memory>
#include "renderer.hpp"


/*struct DestroyglfwWin{

    void operator()(GLFWwindow* ptr){
         glfwDestroyWindow(ptr);
    }

};

typedef std::unique_ptr<GLFWwindow, DestroyglfwWin> smart_GLFWwindow;*/

namespace engine
{
class Engine
{
public:
    Engine();
private:
    Renderer* renderer;
    GLFWwindow* window;
};
}
#endif