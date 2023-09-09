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

wgpu::ShaderModule loadShader(wgpu::Device device, const std::string &name) {
  std::string path = "shaders/" + name + ".wgsl";
  std::string label = name + " shader module";
  std::string shaderWgsl = readFileToString(path);
  wgpu::ShaderModuleWGSLDescriptor wgslDesc;
  wgslDesc.code = shaderWgsl.c_str();
  wgpu::ShaderModuleDescriptor desc{
      .nextInChain = &wgslDesc,
      .label = label.c_str(),
  };
  return device.CreateShaderModule(&desc);
}

int main() {
  // auto GLFW = glfw::init();

  // glfw::WindowHints{.clientApi = glfw::ClientApi::None,
  //                   .cocoaRetinaFramebuffer = false}
  //     .apply();
  // glfw::Window window{640, 480, "TraceG"};

  auto instance = wgpu::CreateInstance();

  // Get Adapter
  wgpu::Adapter adapter;
  instance.RequestAdapter(
      nullptr,
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

  // compute pipeline
  auto computeShader = loadShader(device, "compute");

  wgpu::ComputePipelineDescriptor compPipeDesc{
      .label = "Raytrace pipeline",
      .compute =
          {
              .module = computeShader,
              .entryPoint = "main",
          },
  };
  auto computePipeline = device.CreateComputePipeline(&compPipeDesc);

  wgpu::TextureDescriptor outputTextureDesc{
      .label = "Output texture",
      .usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::StorageBinding,
      .dimension = wgpu::TextureDimension::e2D,
      .size = {16, 16},
      .format = wgpu::TextureFormat::RGBA8Unorm,
      .mipLevelCount = 1,
      .sampleCount = 1,
  };
  auto outputTexture = device.CreateTexture(&outputTextureDesc);
  std::array<wgpu::BindGroupEntry, 1> computeOutputBindGroupDescEntries{
      wgpu::BindGroupEntry{
          .binding = 0,
          .textureView = outputTexture.CreateView(),
      },
  };
  wgpu::BindGroupDescriptor computeOutputBindGroupDesc{
      .label = "Compute Output Bind Group",
      .layout = computePipeline.GetBindGroupLayout(0),
      .entryCount = computeOutputBindGroupDescEntries.size(),
      .entries = computeOutputBindGroupDescEntries.data(),
  };
  auto computeOutputBindGroup =
      device.CreateBindGroup(&computeOutputBindGroupDesc);

  wgpu::CommandEncoderDescriptor encDesc{
      .label = "Compute Encoder",
  };
  auto computeEncoder = device.CreateCommandEncoder(&encDesc);

  {
    wgpu::ComputePassDescriptor passDesc{
        .label = "Compute Pass",
    };
    auto computePass = computeEncoder.BeginComputePass(&passDesc);
    computePass.SetPipeline(computePipeline);
    computePass.SetBindGroup(0, computeOutputBindGroup);
    computePass.DispatchWorkgroups(1, 1);
    computePass.End();
  }

  wgpu::BufferDescriptor outputBufferDesc{
      .label = "Output Buffer",
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = 64 * 64 * 4,
  };
  auto outputBuffer = device.CreateBuffer(&outputBufferDesc);
  wgpu::ImageCopyTexture texSrc{
      .texture = outputTexture,
  };
  wgpu::ImageCopyBuffer bufDst{
      .layout =
          {
              .bytesPerRow = 64 * 4,
          },
      .buffer = outputBuffer,
  };
  wgpu::Extent3D copySize{16, 16};
  computeEncoder.CopyTextureToBuffer(&texSrc, &bufDst, &copySize);

  auto commands = computeEncoder.Finish();
  device.GetQueue().Submit(1, &commands);

  outputBuffer.MapAsync(
      wgpu::MapMode::Read, 0, 64 * 64 * 4,
      [](WGPUBufferMapAsyncStatus cStatus, void *userdata) {
        wgpu::BufferMapAsyncStatus status{cStatus};
        if (status != wgpu::BufferMapAsyncStatus::Success) {
          std::cerr << "map failed?!: " << static_cast<int>(status) << '\n';
        }
        (void)userdata;
      },
      nullptr);

  for (;;) {
    device.Tick();
    if (outputBuffer.GetMapState() == wgpu::BufferMapState::Mapped) {
      const uint8_t *output =
          reinterpret_cast<const uint8_t *>(outputBuffer.GetConstMappedRange());
      for (int i = 0; i < 4 * 128; i++) {
        std::cerr << (uint32_t)output[i] << " ";
      }
      std::cerr << '\n';
      break;
    }
  }

  // dusk::dump_utils::DumpDevice(device);

  // Get surface
  // auto surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

  // // Setup swapchain
  // auto size = window.getSize();
  // wgpu::SwapChainDescriptor swapchainDesc{
  //     .usage = wgpu::TextureUsage::RenderAttachment,
  //     .format = wgpu::TextureFormat::BGRA8Unorm,
  //     .width = static_cast<uint32_t>(std::get<0>(size)),
  //     .height = static_cast<uint32_t>(std::get<1>(size)),
  //     .presentMode = wgpu::PresentMode::Mailbox,
  // };
  // auto swapchain = device.CreateSwapChain(surface, &swapchainDesc);
  //
  // // Create buffers
  // // auto vertexBuffer = dusk::webgpu::createBufferFromData(
  // //     device, "Vertex Buffer", vertex_data, sizeof(vertex_data),
  // //     wgpu::BufferUsage::Vertex);
  //
  // // Shaders
  // auto vertexShader = loadShader(device, "vertex");
  // auto fragmentShader = loadShader(device, "fragment");
  //
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

  // wgpu::ColorTargetState target{
  //     .format = wgpu::TextureFormat::BGRA8Unorm,
  // };
  //
  // wgpu::FragmentState fragState{
  //     .module = fragmentShader,
  //     .entryPoint = "main",
  //     .targetCount = 1,
  //     .targets = &target,
  // };
  //
  // wgpu::RenderPipelineDescriptor pipelineDesc{
  //     .label = "Main Render Pipeline",
  //     .layout = nullptr, // Automatic layout
  //     .vertex =
  //         {
  //             .module = vertexShader, .entryPoint = "main",
  //             // .bufferCount = 1,
  //             // .buffers = &vertBufferLayout,
  //         },
  //     .fragment = &fragState,
  // };
  // auto pipeline = device.CreateRenderPipeline(&pipelineDesc);
  //
  // // Per-frame method
  // auto frame = [&]() {
  //   auto encoder = device.CreateCommandEncoder();
  //   encoder.SetLabel("Main command encoder");
  //
  //   {
  //     auto backbufferView = swapchain.GetCurrentTextureView();
  //     backbufferView.SetLabel("Back Buffer Texture View");
  //
  //     wgpu::RenderPassColorAttachment attachment{
  //         .view = backbufferView,
  //         .loadOp = wgpu::LoadOp::Load,
  //         .storeOp = wgpu::StoreOp::Store,
  //     };
  //
  //     wgpu::RenderPassDescriptor renderPass{
  //         .label = "Main Render Pass",
  //         .colorAttachmentCount = 1,
  //         .colorAttachments = &attachment,
  //     };
  //
  //     auto pass = encoder.BeginRenderPass(&renderPass);
  //     pass.SetPipeline(pipeline);
  //     pass.Draw(6);
  //     pass.End();
  //   }
  //   auto commands = encoder.Finish();
  //
  //   device.GetQueue().Submit(1, &commands);
  //   swapchain.Present();
  // };
  //
  // while (!window.shouldClose()) {
  //   frame();
  //   glfw::pollEvents();
  // }

  return 0;
}
