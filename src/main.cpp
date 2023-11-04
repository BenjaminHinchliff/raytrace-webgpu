#include "hittables/hittable.hpp"
#include "hittables/plane.hpp"
#include "hittables/sphere.hpp"
#include "materials/dielectric.hpp"
#include "materials/lambertian.hpp"
#include "materials/material.hpp"
#include "materials/metal.hpp"
#include "render.hpp"
#include "scene.hpp"

#include <cmrc/cmrc.hpp>
#include <cxxopts.hpp>
#include <glm/vec2.hpp>
#include <stb_image_write.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

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

int main(int argc, char **argv) {
  cxxopts::Options options("traceg",
                           "WebGPU DAWN based GPU-accelerated raytracer");
  // clang-format off
  options.add_options()
    ("o,output", "Output file", cxxopts::value<std::string>())
    ("h,help", "Print usage")
    ;
  // clang-format on
  options.parse_positional({"output"});

  auto result = options.parse(argc, argv);

  if (result.count("help") > 0 || result.count("output") == 0) {
    std::cerr << options.help() << '\n';
    return EXIT_FAILURE;
  }

  auto output_file = result["output"].as<std::string>();

  auto fs = cmrc::shaders::get_filesystem();

  TracerConfig config{
      .size = {1920, 1080},
  };

  auto green = std::make_shared<Lambertian>(glm::vec3{0.8, 0.8, 0.0});
  auto red = std::make_shared<Lambertian>(glm::vec3{0.7, 0.3, 0.3});
  auto glass = std::make_shared<Dielectric>(1.5);
  auto bronze = std::make_shared<Metal>(glm::vec3{0.8, 0.6, 0.2}, 1.0);
  std::vector<std::unique_ptr<Hittable>> hittables;
  hittables.push_back(
      std::make_unique<Sphere>(glm::vec3{0.0, 0.0, -1.0}, 0.5, red));
  hittables.push_back(
      std::make_unique<Sphere>(glm::vec3{-1.0, 0.0, -1.0}, 0.5, glass));
  hittables.push_back(
      std::make_unique<Sphere>(glm::vec3{-1.0, 0.0, -1.0}, -0.4, glass));
  hittables.push_back(
      std::make_unique<Sphere>(glm::vec3{1.0, 0.0, -1.0}, 0.5, bronze));
  hittables.push_back(std::make_unique<Plane>(
      glm::vec3{0.0, -0.5, -1.0}, glm::vec3{0.0, -1.0, 0.0}, green));

  Scene scene{std::move(hittables)};

  auto f = fs.open("compute.wgsl");
  std::string source{f.begin(), f.end()};
  Renderer renderer{source};
  auto props = renderer.adapter_properties();
  std::cerr << "GPU: " << props.name << '\n';
  auto output = renderer.render_scene(std::move(scene), {1920, 1080});

  stbi_write_png(output_file.c_str(), config.size.x, config.size.y, 4,
                 output.data(), config.size.x * 4);

  return EXIT_SUCCESS;
}
