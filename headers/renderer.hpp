#include <webgpu/webgpu.hpp>
#include <memory>
#include <glfw/glfw3.h>

class Renderer
{
public:
    Renderer(GLFWwindow* window);
    ~Renderer();
    void render(WGPUColor color);
    std::unique_ptr<WGPUDevice> device;
private:
    std::unique_ptr<WGPUSwapChain> swapChain;
    std::unique_ptr<WGPUAdapter> adapter;
    std::unique_ptr<WGPUSurface> surface;
    std::unique_ptr<WGPUInstance> instance;
    std::unique_ptr<WGPURenderPipeline> pipeline;
    std::unique_ptr<WGPUQueue> queue;
};