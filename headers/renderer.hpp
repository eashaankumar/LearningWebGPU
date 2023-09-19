#include <webgpu/webgpu.hpp>
#include <memory>
#include <glfw/glfw3.h>

/*struct DestroyglfwWin{

    void operator()(GLFWwindow* ptr){
         glfwDestroyWindow(ptr);
    }

};

typedef std::unique_ptr<GLFWwindow, DestroyglfwWin> smart_GLFWwindow;*/

class Renderer
{
public:
    Renderer(GLFWwindow*);
    void render(WGPUColor color);
    void cleanup();
private:
    std::unique_ptr<WGPUSwapChain> swapChain;
    std::unique_ptr<WGPUDevice> device;
    std::unique_ptr<WGPUAdapter> adapter;
    std::unique_ptr<WGPUSurface> surface;
    std::unique_ptr<WGPUInstance> instance;
    std::unique_ptr<WGPURenderPipeline> pipeline;
    std::unique_ptr<WGPUQueue> queue;
};