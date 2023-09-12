#include "render.hpp"
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
  auto fs = cmrc::shaders::get_filesystem();

  TracerConfig config{
      .size = {1920, 1080},
  };

  auto f = fs.open("compute.wgsl");
  std::string source{f.begin(), f.end()};
  Renderer renderer{source};
  auto output = renderer.render_scene({1920, 1080});

  stbi_write_png("out.png", config.size.x, config.size.y, 4, output.data(),
                 config.size.x * 4);

  return 0;
}
