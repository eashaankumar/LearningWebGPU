#include <glfw/glfw3.h>
#include <glfw3webgpu.h>
#include "renderer.hpp"
#include "utils.hpp"

void setDefault(WGPULimits &limits) {
    limits.maxTextureDimension1D = 0;
    limits.maxTextureDimension2D = 0;
    limits.maxTextureDimension3D = 0;
    limits.maxBufferSize = 0;
    // [...] Set everything to 0 to mean "no limit"
}

void setDefaults(WGPUAdapter adapter, WGPUDevice device)
{
    WGPUSupportedLimits supportedLimits{};
    supportedLimits.nextInChain = nullptr;

    wgpuAdapterGetLimits(adapter, &supportedLimits);
    std::cout << "adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

    wgpuDeviceGetLimits(device, &supportedLimits);
    std::cout << "device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;
}

Renderer::Renderer(GLFWwindow* window)
{

    #pragma region Init WebGPU
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    instance = std::make_unique<WGPUInstance>(wgpuCreateInstance(&desc));

    if(!instance)
    {
        std::cerr << "Could not init webgpu" << std::endl;
        glfwTerminate();
        throw std::exception();
    }

    std::cout << "WGPU instance: " << instance << std::endl;
    
    #pragma region adapter
    std::cout << "Requesting adapter..." << std::endl;

    surface = std::make_unique<WGPUSurface>(glfwGetWGPUSurface(*instance, window ));
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = *surface;
    adapter = std::make_unique<WGPUAdapter>(requestAdapter(*instance, &adapterOpts));

    std::cout << "Got adapter: " << adapter << std::endl;
    #pragma endregion

    #pragma region features
    std::vector<WGPUFeatureName> features;
    size_t featureCount = wgpuAdapterEnumerateFeatures(*adapter, nullptr);
    features.resize(featureCount);
    wgpuAdapterEnumerateFeatures(*adapter, features.data());
    std::cout << "Adapter features: " << std:: endl;
    for(auto f : features)
    {
        std::cout << " - " << f << std::endl;
    }
    #pragma endregion

    #pragma region device
    std::cout << "Requesting device..." << std::endl;

    WGPUSupportedLimits supportedLimits;
    wgpuAdapterGetLimits(*adapter, &supportedLimits);

	std::cout << "Requesting device..." << std::endl;
	// Don't forget to = Default
	WGPURequiredLimits requiredLimits; // = Default?
	// We use at most 1 vertex attribute for now
	requiredLimits.limits.maxVertexAttributes = 1;
	// We should also tell that we use 1 vertex buffers
	requiredLimits.limits.maxVertexBuffers = 1;
	// Maximum size of a buffer is 6 vertices of 2 float each
	requiredLimits.limits.maxBufferSize = 6 * 2 * sizeof(float);
	// Maximum stride between 2 consecutive vertices in the vertex buffer
	requiredLimits.limits.maxVertexBufferArrayStride = 2 * sizeof(float);
	// This must be set even if we do not use storage buffers for now
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	// This must be set even if we do not use uniform buffers for now
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    
    WGPUDeviceDescriptor deviceDesc = {};
    
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = &requiredLimits; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";

    device = std::make_unique<WGPUDevice>(requestDevice(*adapter, &deviceDesc));

    std::cout << "Got device: " << device << std::endl;
    setDefaults(*adapter, *device);

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(*device, onDeviceError, nullptr /* pUserData */);
    #pragma endregion

    #pragma region command queue
    queue = std::make_unique<WGPUQueue>(wgpuDeviceGetQueue(*device));
    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
    std::cout << "Queued work finished with status: " << status << std::endl;
    };
    uint64_t signalValue = 0;
    wgpuQueueOnSubmittedWorkDone(*queue, signalValue , onQueueWorkDone, nullptr /* pUserData */);
    #pragma endregion

    #pragma region swap chain
    WGPUSwapChainDescriptor swapChainDesc = {};
    swapChainDesc.nextInChain = nullptr;
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    WGPUTextureFormat swapChainFormat = WGPUTextureFormat_BGRA8Unorm;//wgpuSurfaceGetPreferredFormat(surface, adapter);
    swapChainDesc.format = swapChainFormat;
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;
    swapChain = std::make_unique<WGPUSwapChain>(wgpuDeviceCreateSwapChain(*device, *surface, &swapChainDesc));
    std::cout << "Swapchain: " << swapChain << std::endl;
    #pragma endregion

    #pragma region Shader
    	std::cout << "Creating shader module..." << std::endl;
	const char* shaderSource = R"(
@vertex
fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
    return vec4f(in_vertex_position, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 1.0, 1.0, 1.0);
}
)";

	WGPUShaderModuleDescriptor shaderDesc = {};
	shaderDesc.nextInChain = nullptr;
#ifdef WEBGPU_BACKEND_WGPU
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#endif

	// Use the extension mechanism to load a WGSL shader source code
	WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
	// Set the chained struct's header
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	// Connect the chain
	shaderDesc.nextInChain = &shaderCodeDesc.chain;

	// Setup the actual payload of the shader code descriptor
	shaderCodeDesc.code = shaderSource;

    // Vertex fetch
    WGPUVertexAttribute vertexAttrib;
    // == Per attribute ==
    // Corresponds to @location(...)
    vertexAttrib.shaderLocation = 0;
    // Means vec2f in the shader
    vertexAttrib.format = WGPUVertexFormat_Float32x2;
    // Index of the first element
    vertexAttrib.offset = 0;

    WGPUVertexBufferLayout vertexBufferLayout{};
    //vertexBufferLayout.nextInChain = nullptr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.attributes = &vertexAttrib;
    // == Common to attributes from the same buffer ==
    vertexBufferLayout.arrayStride = 2 * sizeof(float);
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
    // [...] Build vertex buffer layout

	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(*device, &shaderDesc);
	std::cout << "Shader module: " << shaderModule << std::endl;
	std::cout << "Creating render pipeline..." << std::endl;
	WGPURenderPipelineDescriptor pipelineDesc = {};
	pipelineDesc.nextInChain = nullptr;

	// Vertex fetch
	// (We don't use any input buffer so far)
	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	// Vertex shader
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	// Primitive assembly and rasterization
	// Each sequence of 3 vertices is considered as a triangle
	pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	// We'll see later how to specify the order in which vertices should be
	// connected. When not specified, vertices are considered sequentially.
	pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
	// The face orientation is defined by assuming that when looking
	// from the front of the face, its corner vertices are enumerated
	// in the counter-clockwise (CCW) order.
	pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
	// But the face orientation does not matter much because we do not
	// cull (i.e. "hide") the faces pointing away from us (which is often
	// used for optimization).
	pipelineDesc.primitive.cullMode = WGPUCullMode_None;

	// Fragment shader
	WGPUFragmentState fragmentState = {};
	fragmentState.nextInChain = nullptr;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	// Configure blend state
	WGPUBlendState blendState;
	// Usual alpha blending for the color:
	blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
	blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	blendState.color.operation = WGPUBlendOperation_Add;
	// We leave the target alpha untouched:
	blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
	blendState.alpha.dstFactor = WGPUBlendFactor_One;
	blendState.alpha.operation = WGPUBlendOperation_Add;

	WGPUColorTargetState colorTarget = {};
	colorTarget.nextInChain = nullptr;
	colorTarget.format = swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

	// We have only one target because our render pass has only one output color
	// attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	
	// Depth and stencil tests are not used here
	pipelineDesc.depthStencil = nullptr;

	// Multi-sampling
	// Samples per pixel
	pipelineDesc.multisample.count = 1;
	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;
	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Pipeline layout
	pipelineDesc.layout = nullptr;

	pipeline = std::make_unique<WGPURenderPipeline>(wgpuDeviceCreateRenderPipeline(*device, &pipelineDesc));
	std::cout << "Render pipeline: " << pipeline << std::endl;
    #pragma endregion

    #pragma endregion

}

Renderer::~Renderer()
{
    wgpuSwapChainRelease(*swapChain);
    wgpuDeviceRelease(*device);
    wgpuAdapterRelease(*adapter);
    wgpuSurfaceRelease(*surface);
    wgpuInstanceRelease(*instance);
    wgpuQueueReference(*queue);
    // remove pipeline from device?    
}

void draw(WGPUQueue queue, WGPUDevice device, WGPURenderPassEncoder renderPass)
{
    // Vertex buffer
    // There are 2 floats per vertex, one for x and one for y.
    // But in the end this is just a bunch of floats to the eyes of the GPU,
    // the *layout* will tell how to interpret this.
    std::vector<float> vertexData = {
        // x0, y0
        -0.5, -0.5,

        // x1, y1
        +0.5, -0.5,

        // x2, y2
        +0.0, +0.5
    };
    int vertexCount = static_cast<int>(vertexData.size() / 2);
    // Create vertex buffer
    WGPUBufferDescriptor bufferDesc{};
    bufferDesc.nextInChain = nullptr;
    bufferDesc.size = vertexData.size() * sizeof(float);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDesc.mappedAtCreation = false;
    WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

    // Upload geometry data to the buffer
    wgpuQueueWriteBuffer(queue, vertexBuffer, 0, vertexData.data(), bufferDesc.size);

    // Set vertex buffer while encoding the render pass
    wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, vertexData.size() * sizeof(float));

    // We use the `vertexCount` variable instead of hard-coding the vertex count
    wgpuRenderPassEncoderDraw(renderPass, vertexCount, 1, 0, 0);
}

void Renderer::render(WGPUColor clearColor)
{
    WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(*swapChain);
    if (!nextTexture) {
        std::cerr << "Cannot acquire next swap chain texture" << std::endl;
        throw std::exception();
    }

    WGPUCommandEncoderDescriptor commandEncoderDesc = {};
    commandEncoderDesc.nextInChain = nullptr;
    commandEncoderDesc.label = "Command Encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(*device, &commandEncoderDesc);
    
    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;

    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = clearColor;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;
    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    // In its overall outline, drawing a triangle is as simple as this:
    // Select which render pipeline to use
    wgpuRenderPassEncoderSetPipeline(renderPass, *pipeline);
    // Draw 1 instance of a 3-vertices shape
    //wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
    draw(*queue, *device, renderPass);

    wgpuRenderPassEncoderEnd(renderPass);
    
    wgpuTextureViewRelease(nextTexture);

    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuQueueSubmit(*queue, 1, &command);

    wgpuSwapChainPresent(*swapChain);
}