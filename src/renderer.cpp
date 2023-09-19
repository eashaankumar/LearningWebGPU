#include <glfw/glfw3.h>
#include <glfw3webgpu.h>
#include "renderer.hpp"
#include "utils.hpp"

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

    WGPUDeviceDescriptor deviceDesc = {};
    device = std::make_unique<WGPUDevice>(requestDevice(*adapter, &deviceDesc));

    std::cout << "Got device: " << device << std::endl;

    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";

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
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
	var p = vec2f(0.0, 0.0);
	if (in_vertex_index == 0u) {
		p = vec2f(-0.5, -0.5);
	} else if (in_vertex_index == 1u) {
		p = vec2f(0.5, -0.5);
	} else {
		p = vec2f(0.0, 0.5);
	}
	return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
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

	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(*device, &shaderDesc);
	std::cout << "Shader module: " << shaderModule << std::endl;
	std::cout << "Creating render pipeline..." << std::endl;
	WGPURenderPipelineDescriptor pipelineDesc = {};
	pipelineDesc.nextInChain = nullptr;

	// Vertex fetch
	// (We don't use any input buffer so far)
	pipelineDesc.vertex.bufferCount = 0;
	pipelineDesc.vertex.buffers = nullptr;

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

void Renderer::cleanup()
{
    wgpuSwapChainRelease(*swapChain);
    wgpuDeviceRelease(*device);
    wgpuAdapterRelease(*adapter);
    wgpuSurfaceRelease(*surface);
    wgpuInstanceRelease(*instance);
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
    wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(renderPass);
    
    wgpuTextureViewRelease(nextTexture);

    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuQueueSubmit(*queue, 1, &command);

    wgpuSwapChainPresent(*swapChain);
}