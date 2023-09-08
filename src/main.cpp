#include <cstdlib>
#include <fstream>
#include <glfwpp/glfwpp.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include <cstdint>
#include <iostream>

void Error(WGPUErrorType type, const char *msg, void *) {
  switch (type) {
  case WGPUErrorType_OutOfMemory:
    std::cerr << "[Error] Out Of Memory: " << msg << std::endl;
    abort();
  case WGPUErrorType_Validation:
    std::cerr << "[Error Validation: " << msg << std::endl;
    abort();
  case WGPUErrorType_NoError:
  case WGPUErrorType_Unknown:
  case WGPUErrorType_DeviceLost:
  case WGPUErrorType_Force32:
  case WGPUErrorType_Internal:
    std::cerr << msg << std::endl;
    break;
  }
}

void DeviceLost(WGPUDeviceLostReason reason, char const *msg, void *) {
  std::cerr << "[Device Lost]: ";
  switch (reason) {
  case WGPUDeviceLostReason_Undefined:
    std::cerr << "Undefined: " << msg << std::endl;
    break;
  case WGPUDeviceLostReason_Destroyed:
    std::cerr << "Destroyed: " << msg << std::endl;
    break;
  case WGPUDeviceLostReason_Force32:
    std::cerr << "Force32: " << msg << std::endl;
    break;
  }
}

std::string_view backendTypeString(wgpu::BackendType backendType) {
  using wgpu::BackendType;
  switch (backendType) {
  case BackendType::Undefined:
    return "Undefined";
  case BackendType::Null:
    return "Null";
  case BackendType::D3D11:
    return "D3D11";
  case BackendType::D3D12:
    return "D3D12";
  case BackendType::Metal:
    return "Metal";
  case BackendType::OpenGL:
    return "OpenGL";
  case BackendType::Vulkan:
    return "Vulkan";
  case BackendType::WebGPU:
    return "WebGPU";
  case BackendType::OpenGLES:
    return "OpenGLES";
  }
}

void Logging(WGPULoggingType type, const char *msg, void *) {
  switch (type) {
  case WGPULoggingType_Verbose:
    std::cerr << "Log [Verbose]: ";
    break;
  case WGPULoggingType_Info:
    std::cerr << "Log [Info]: ";
    break;
  case WGPULoggingType_Warning:
    std::cerr << "Log [Warning]: ";
    break;
  case WGPULoggingType_Error:
    std::cerr << "Log [Error]: ";
    break;
  case WGPULoggingType_Force32:
    std::cerr << "Log [Force32]: ";
    break;
  }
  std::cerr << msg << std::endl;
}

std::string readFileToString(const std::string &path) {
  std::ifstream t{path};
  if (!t) {
    throw std::runtime_error("failed to open file " + path);
  }
  std::stringstream buffer;
  buffer << t.rdbuf();
  return buffer.str();
}

int main() {
  auto GLFW = glfw::init();

  glfw::WindowHints{.clientApi = glfw::ClientApi::None,
                    .cocoaRetinaFramebuffer = false}
      .apply();
  glfw::Window window{640, 480, "TraceG"};

  auto instance = wgpu::CreateInstance();

  // Get Adapter
  wgpu::Adapter adapter;
  wgpu::RequestAdapterOptions options{
      .powerPreference = wgpu::PowerPreference::HighPerformance,
  };
  instance.RequestAdapter(
      &options,
      [](WGPURequestAdapterStatus, WGPUAdapter adapterIn, const char *,
         void *userdata) {
        *static_cast<wgpu::Adapter *>(userdata) =
            wgpu::Adapter::Acquire(adapterIn);
      },
      &adapter);

  wgpu::AdapterProperties adapterProps;
  adapter.GetProperties(&adapterProps);

  std::cout << adapterProps.name << ": "
            << backendTypeString(adapterProps.backendType) << '\n';

  // dusk::dump_utils::DumpAdapter(adapter);

  // Get device
  auto device = adapter.CreateDevice();
  device.SetLabel("Primary Device");

  device.SetUncapturedErrorCallback(Error, nullptr);
  device.SetDeviceLostCallback(DeviceLost, nullptr);
  // Logging is enabled as soon as the callback is setup.
  device.SetLoggingCallback(Logging, nullptr);

  // dusk::dump_utils::DumpDevice(device);

  // Get surface
  auto surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

  // Setup swapchain
  auto size = window.getSize();
  wgpu::SwapChainDescriptor swapchainDesc{
      .usage = wgpu::TextureUsage::RenderAttachment,
      .format = wgpu::TextureFormat::BGRA8Unorm,
      .width = static_cast<uint32_t>(std::get<0>(size)),
      .height = static_cast<uint32_t>(std::get<1>(size)),
      .presentMode = wgpu::PresentMode::Mailbox,
  };
  auto swapchain = device.CreateSwapChain(surface, &swapchainDesc);

  // Create buffers
  // auto vertexBuffer = dusk::webgpu::createBufferFromData(
  //     device, "Vertex Buffer", vertex_data, sizeof(vertex_data),
  //     wgpu::BufferUsage::Vertex);

  // Shaders
  std::string vertexShaderWgsl = readFileToString("shaders/vertex.wgsl");
  wgpu::ShaderModuleWGSLDescriptor vertexWgslDesc;
  vertexWgslDesc.code = vertexShaderWgsl.c_str();
  wgpu::ShaderModuleDescriptor vertexDesc{
      .nextInChain = &vertexWgslDesc,
      .label = "Vertex Shader Module",
  };

  std::string fragmentShaderWgsl = readFileToString("shaders/fragment.wgsl");
  wgpu::ShaderModuleWGSLDescriptor fragmentWgslDesc;
  fragmentWgslDesc.code = fragmentShaderWgsl.c_str();
  wgpu::ShaderModuleDescriptor fragmentDesc{
      .nextInChain = &fragmentWgslDesc,
      .label = "Fragment Shader Module",
  };

  auto vertexShader = device.CreateShaderModule(&vertexDesc);
  auto fragmentShader = device.CreateShaderModule(&fragmentDesc);

  // Pipeline creation
  // wgpu::VertexAttribute vertAttributes[2] = {
  //     {
  //         .format = wgpu::VertexFormat::Float32x4,
  //         .offset = 0,
  //         .shaderLocation = 0,
  //     },
  //     {
  //         .format = wgpu::VertexFormat::Float32x4,
  //         .offset = 4 * sizeof(float),
  //         .shaderLocation = 1,
  //     }};
  //
  // wgpu::VertexBufferLayout vertBufferLayout{
  //     .arrayStride = 8 * sizeof(float),
  //     .attributeCount = 2,
  //     .attributes = vertAttributes,
  // };

  wgpu::ColorTargetState target{
      .format = wgpu::TextureFormat::BGRA8Unorm,
  };

  wgpu::FragmentState fragState{
      .module = fragmentShader,
      .entryPoint = "main",
      .targetCount = 1,
      .targets = &target,
  };

  wgpu::RenderPipelineDescriptor pipelineDesc{
      .label = "Main Render Pipeline",
      .layout = nullptr, // Automatic layout
      .vertex =
          {
              .module = vertexShader, .entryPoint = "main",
              // .bufferCount = 1,
              // .buffers = &vertBufferLayout,
          },
      .fragment = &fragState,
  };
  auto pipeline = device.CreateRenderPipeline(&pipelineDesc);

  // Per-frame method
  auto frame = [&]() {
    auto encoder = device.CreateCommandEncoder();
    encoder.SetLabel("Main command encoder");

    {
      auto backbufferView = swapchain.GetCurrentTextureView();
      backbufferView.SetLabel("Back Buffer Texture View");

      wgpu::RenderPassColorAttachment attachment{
          .view = backbufferView,
          .loadOp = wgpu::LoadOp::Load,
          .storeOp = wgpu::StoreOp::Store,
      };

      wgpu::RenderPassDescriptor renderPass{
          .label = "Main Render Pass",
          .colorAttachmentCount = 1,
          .colorAttachments = &attachment,
      };

      auto pass = encoder.BeginRenderPass(&renderPass);
      pass.SetPipeline(pipeline);
      pass.Draw(6);
      pass.End();
    }
    auto commands = encoder.Finish();

    device.GetQueue().Submit(1, &commands);
    swapchain.Present();
  };

  while (!window.shouldClose()) {
    frame();
    glfw::pollEvents();
  }

  return 0;
}
