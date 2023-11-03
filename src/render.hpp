#ifndef RENDER_H_
#define RENDER_H_

#include "scene.hpp"

#include <glm/vec2.hpp>
#include <webgpu/webgpu_cpp.h>

#include <string>
#include <vector>

class Renderer {
public:
  Renderer(std::string source);

  wgpu::AdapterProperties adapter_properties() const;
  std::vector<uint8_t> render_scene(Scene scene, glm::uvec2 size);

private:
  wgpu::Adapter
  request_adapter(const wgpu::RequestAdapterOptions &options) const;
  wgpu::Device setup_device(const wgpu::Adapter adapter) const;

private:
  std::string source;
  wgpu::Instance instance;
  wgpu::Adapter adapter;
  wgpu::Device device;
};

#endif // !RENDER_H_
