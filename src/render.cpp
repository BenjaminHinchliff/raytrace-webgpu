#include "render.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#define EXPLICIT_UNUSED(ident) (void)ident

namespace logging {
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
} // namespace logging

wgpu::Adapter
Renderer::request_adapter(const wgpu::RequestAdapterOptions &options) const {
  wgpu::Adapter adapter;
  instance.RequestAdapter(
      &options,
      [](WGPURequestAdapterStatus status, WGPUAdapter adapterIn,
         const char *msg, void *userdata) {
        using namespace std::string_literals;
        if (wgpu::RequestAdapterStatus{status} !=
            wgpu::RequestAdapterStatus::Success) {
          throw std::runtime_error("Failed to aquire adapter: "s + msg);
        }
        *static_cast<wgpu::Adapter *>(userdata) =
            wgpu::Adapter::Acquire(adapterIn);
      },
      &adapter);
  return adapter;
}

wgpu::Device Renderer::setup_device(const wgpu::Adapter adapter) const {
  wgpu::Device device = adapter.CreateDevice();
  device.SetLabel("Primary Device");
  device.SetUncapturedErrorCallback(logging::Error, nullptr);
  device.SetDeviceLostCallback(logging::DeviceLost, nullptr);
  device.SetLoggingCallback(logging::Logging, nullptr);
  return device;
}

Renderer::Renderer(std::string source)
    : source{source}, instance{wgpu::CreateInstance()} {
  // Get Adapter
  wgpu::RequestAdapterOptions adapterOpts{
      .powerPreference = wgpu::PowerPreference::HighPerformance,
  };
  adapter = request_adapter(adapterOpts);
  device = setup_device(adapter);
}

wgpu::AdapterProperties Renderer::adapter_properties() const {
  wgpu::AdapterProperties adapterProps;
  adapter.GetProperties(&adapterProps);
  return adapterProps;
}

uint32_t paddedBytesPerRow(uint32_t width) {
  // This is defined inside of DAWN and the WebGPU spec and isn't reasonable to
  // pull from them
  constexpr uint32_t BYTES_PER_ROW_ALIGN = 256;
  uint32_t bytesPerRow = width * 4;
  uint32_t padding = (BYTES_PER_ROW_ALIGN - bytesPerRow % BYTES_PER_ROW_ALIGN) %
                     BYTES_PER_ROW_ALIGN;
  return bytesPerRow + padding;
}

glm::uvec2 calculateWorkgroups(glm::uvec2 size) {
  // this has to be hardcoded because it is also in compute.wgsl
  constexpr glm::uvec2 WORKGROUP_SIZE{16, 16};
  return (size + WORKGROUP_SIZE - glm::uvec2{1}) / WORKGROUP_SIZE;
}

wgpu::ShaderModule create_shader(wgpu::Device device,
                                 const std::string &source) {
  wgpu::ShaderModuleWGSLDescriptor wgslDesc;
  wgslDesc.code = source.c_str();
  wgpu::ShaderModuleDescriptor desc{
      .nextInChain = &wgslDesc,
      .label = "compute shader module",
  };
  return device.CreateShaderModule(&desc);
}

struct RenderConfig {
  uint32_t samples_per_pixel;
  uint32_t max_depth;
};

std::vector<uint8_t> Renderer::render_scene(Scene scene, glm::uvec2 size,
                                            uint32_t samples,
                                            uint32_t max_depth) {
  std::string sourceWithScene{scene.generate() + source};
  auto computeShader = create_shader(device, sourceWithScene);

  wgpu::ComputePipelineDescriptor compPipeDesc{
      .label = "Raytrace pipeline",
      .compute =
          {
              .module = computeShader,
              .entryPoint = "main",
          },
  };
  auto computePipeline = device.CreateComputePipeline(&compPipeDesc);

  wgpu::Extent3D outputExtent{size.x, size.y};
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

  std::array<wgpu::BindGroupLayoutEntry, 1> configBindGroupLayoutEntries{
      wgpu::BindGroupLayoutEntry{
          .binding = 0,
          .visibility = wgpu::ShaderStage::Compute,
          .buffer =
              wgpu::BufferBindingLayout{
                  .type = wgpu::BufferBindingType::Uniform,
              },
      },
  };
  wgpu::BindGroupLayoutDescriptor configBindGroupLayoutDesc{
      .label = "Config Bind Group Layout",
      .entryCount = configBindGroupLayoutEntries.size(),
      .entries = configBindGroupLayoutEntries.data(),
  };
  auto configBindGroupLayout =
      device.CreateBindGroupLayout(&configBindGroupLayoutDesc);

  RenderConfig config{
      .samples_per_pixel = samples,
      .max_depth = max_depth,
  };
  wgpu::BufferDescriptor configBufferDesc{
      .label = "Config Buffer",
      .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
      .size = sizeof(RenderConfig),
      .mappedAtCreation = true,
  };
  auto configBuffer = device.CreateBuffer(&configBufferDesc);
  *static_cast<RenderConfig *>(configBuffer.GetMappedRange()) = config;
  configBuffer.Unmap();

  std::array<wgpu::BindGroupEntry, 1> configBindGroupEntries{
      wgpu::BindGroupEntry{
          .binding = 0,
          .buffer = configBuffer,
      },
  };
  wgpu::BindGroupDescriptor configBindGroupDesc{
      .label = "Config Bind Group",
      .layout = computePipeline.GetBindGroupLayout(1),
      .entryCount = configBindGroupEntries.size(),
      .entries = configBindGroupEntries.data(),
  };
  auto configBindGroup = device.CreateBindGroup(&configBindGroupDesc);

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
    computePass.SetBindGroup(1, configBindGroup);
    glm::uvec2 workgroups = calculateWorkgroups(size);
    computePass.DispatchWorkgroups(workgroups.x, workgroups.y);
    computePass.End();
  }

  uint32_t bytesPerRow = paddedBytesPerRow(size.x);
  wgpu::BufferDescriptor outputBufferDesc{
      .label = "Output Buffer",
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = bytesPerRow * size.y,
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
      wgpu::MapMode::Read, 0, bytesPerRow * size.y,
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
  output.reserve(size.x * size.y * 4);
  for (size_t y = 0; y < size.y; y++) {
    auto start = &paddedOutput[bytesPerRow * y];
    output.insert(output.end(), start, start + (size.x * 4));
  }
  return output;
}
