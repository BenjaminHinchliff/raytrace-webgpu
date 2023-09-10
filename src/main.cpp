#include <cmrc/cmrc.hpp>

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <glm/vec2.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

CMRC_DECLARE(shaders);

class TracerConfig {
public:
  glm::uvec2 size;
};

#define EXPLICIT_UNUSED(ident) (void)ident

void Error(WGPUErrorType type, const char *msg, void *) {
  switch (type) {
  case WGPUErrorType_OutOfMemory:
    std::cerr << "[Error] Out Of Memory: " << msg << '\n';
    abort();
  case WGPUErrorType_Validation:
    std::cerr << "[Error Validation: " << msg << '\n';
    abort();
  case WGPUErrorType_NoError:
  case WGPUErrorType_Unknown:
  case WGPUErrorType_DeviceLost:
  case WGPUErrorType_Force32:
  case WGPUErrorType_Internal:
    std::cerr << msg << '\n';
    break;
  }
}

void DeviceLost(WGPUDeviceLostReason reason, char const *msg, void *) {
  std::cerr << "[Device Lost]: ";
  switch (reason) {
  case WGPUDeviceLostReason_Undefined:
    std::cerr << "Undefined: " << msg << '\n';
    break;
  case WGPUDeviceLostReason_Destroyed:
    std::cerr << "Destroyed: " << msg << '\n';
    break;
  case WGPUDeviceLostReason_Force32:
    std::cerr << "Force32: " << msg << '\n';
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
  std::cerr << msg << '\n';
}

constexpr uint32_t BYTES_PER_ROW_ALIGN = 256;

uint32_t paddedBytesPerRow(uint32_t width) {
  uint32_t bytesPerRow = width * 4;
  uint32_t padding = (BYTES_PER_ROW_ALIGN - bytesPerRow % BYTES_PER_ROW_ALIGN) %
                     BYTES_PER_ROW_ALIGN;
  return bytesPerRow + padding;
}

constexpr glm::uvec2 WORKGROUP_SIZE{16, 16};

glm::uvec2 calculateWorkgroups(glm::uvec2 size) {
  return (size + WORKGROUP_SIZE - glm::uvec2{1}) / WORKGROUP_SIZE;
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

wgpu::ShaderModule loadShader(wgpu::Device device,
                              cmrc::embedded_filesystem &fs,
                              const std::string &name) {
  std::string path = name + ".wgsl";
  std::string label = name + " shader module";
  auto f = fs.open(path);
  wgpu::ShaderModuleWGSLDescriptor wgslDesc;
  std::string code{f.begin(), f.end()};
  wgslDesc.code = code.c_str();
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

  auto fs = cmrc::shaders::get_filesystem();

  TracerConfig config{
      .size = {1920, 1080},
  };

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
  auto computeShader = loadShader(device, fs, "compute");

  wgpu::ComputePipelineDescriptor compPipeDesc{
      .label = "Raytrace pipeline",
      .compute =
          {
              .module = computeShader,
              .entryPoint = "main",
          },
  };
  auto computePipeline = device.CreateComputePipeline(&compPipeDesc);

  wgpu::Extent3D outputExtent{config.size.x, config.size.y};
  wgpu::TextureDescriptor outputTextureDesc{
      .label = "Output texture",
      .usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::StorageBinding,
      .dimension = wgpu::TextureDimension::e2D,
      .size = outputExtent,
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
    glm::uvec2 workgroups = calculateWorkgroups(config.size);
    computePass.DispatchWorkgroups(workgroups.x, workgroups.y);
    computePass.End();
  }

  uint32_t bytesPerRow = paddedBytesPerRow(config.size.x);
  wgpu::BufferDescriptor outputBufferDesc{
      .label = "Output Buffer",
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = bytesPerRow * config.size.y,
  };
  auto outputBuffer = device.CreateBuffer(&outputBufferDesc);
  wgpu::ImageCopyTexture texSrc{
      .texture = outputTexture,
  };
  wgpu::ImageCopyBuffer bufDst{
      .layout =
          {
              .bytesPerRow = bytesPerRow,
          },
      .buffer = outputBuffer,
  };
  computeEncoder.CopyTextureToBuffer(&texSrc, &bufDst, &outputExtent);

  auto commands = computeEncoder.Finish();
  device.GetQueue().Submit(1, &commands);

  outputBuffer.MapAsync(
      wgpu::MapMode::Read, 0, bytesPerRow * config.size.y,
      [](WGPUBufferMapAsyncStatus cStatus, void *userdata) {
        EXPLICIT_UNUSED(userdata);
        wgpu::BufferMapAsyncStatus status{cStatus};
        if (status != wgpu::BufferMapAsyncStatus::Success) {
          std::cerr << "map failed: " << static_cast<int>(status) << '\n';
        }
      },
      nullptr);

  std::cerr << "waiting on render..." << '\n';

  // so there seems to be no way to poll the dawn device and just wait until
  // the gpu has finished it's work so we just check when the buffer is
  // successfully mapped (guaranteed to be after the pipeline) and then
  // call it a day from there.
  const uint8_t *paddedOutput;
  bool complete = false;
  while (!complete) {
    device.Tick();
    if (outputBuffer.GetMapState() == wgpu::BufferMapState::Mapped) {
      paddedOutput =
          static_cast<const uint8_t *>(outputBuffer.GetConstMappedRange());
      complete = true;
    }
  }

  std::vector<uint8_t> output;
  output.reserve(config.size.x * config.size.y * 4);
  for (size_t y = 0; y < config.size.y; y++) {
    auto start = &paddedOutput[bytesPerRow * y];
    output.insert(output.end(), start, start + (config.size.x * 4));
  }

  stbi_write_png("out.png", config.size.x, config.size.y, 4, output.data(),
                 config.size.x * 4);

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
