#include "hittables/hittable.hpp"
#include "hittables/plane.hpp"
#include "hittables/sphere.hpp"
#include "materials/lambertian.hpp"
#include "materials/material.hpp"
#include "render.hpp"
#include "scene.hpp"

#include <cmrc/cmrc.hpp>
#include <glm/vec2.hpp>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

CMRC_DECLARE(shaders);

class TracerConfig {
public:
  glm::uvec2 size;
};

int main() {
  auto fs = cmrc::shaders::get_filesystem();

  TracerConfig config{
      .size = {1920, 1080},
  };

  std::shared_ptr<Material> red =
      std::make_shared<Lambertian>(glm::vec3{0.7, 0.3, 0.3});
  std::vector<std::unique_ptr<Hittable>> hittables;
  hittables.push_back(
      std::make_unique<Sphere>(glm::vec3{0.0, 0.0, -1.0}, 0.5, red));
  hittables.push_back(std::make_unique<Plane>(glm::vec3{0.0, -0.5, -1.0},
                                              glm::vec3{0.0, -1.0, 0.0}, red));
  for (int i = 0; i < 5; i++) {
    hittables.push_back(std::make_unique<Sphere>(
        glm::vec3{-1.0 + 0.5 * i, -0.4, -0.5}, 0.1, red));
  }

  Scene scene{std::move(hittables)};

  auto f = fs.open("compute.wgsl");
  std::string source{f.begin(), f.end()};
  Renderer renderer{source};
  auto props = renderer.adapter_properties();
  std::cerr << "GPU: " << props.name << '\n';
  auto output = renderer.render_scene(std::move(scene), {1920, 1080});

  stbi_write_png("out.png", config.size.x, config.size.y, 4, output.data(),
                 config.size.x * 4);

  return 0;
}
