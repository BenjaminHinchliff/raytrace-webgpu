#include "hittables/hittable.hpp"
#include "hittables/plane.hpp"
#include "hittables/sphere.hpp"
#include "load.hpp"
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
#include <ranges>
#include <sstream>
#include <string>
#include <utility>

CMRC_DECLARE(shaders);

class TracerConfig {
public:
  glm::uvec2 size;
};

glm::uvec2 parse_dims(const std::string &dims_str) {
  auto x_loc = dims_str.find("x");
  auto width = std::stoi(dims_str.substr(0, x_loc));
  auto height = std::stoi(dims_str.substr(x_loc + 1));
  return {width, height};
}

int main(int argc, char **argv) {
  cxxopts::Options options("traceg",
                           "WebGPU DAWN based GPU-accelerated raytracer");
  // clang-format off
  options.add_options()
    ("s,scene", "Scene input file", cxxopts::value<std::string>())
    ("o,output", "Output file", cxxopts::value<std::string>())
    ("d,dims", "Dimensions of the output image in format WxH (e.g. 1920x1080)",
     cxxopts::value<std::string>()->default_value("640x480"))
    ("h,help", "Print usage")
    ;
  // clang-format on
  options.parse_positional({"scene", "output"});
  options.positional_help("<SCENE> <OUTPUT>").show_positional_help();

  auto result = options.parse(argc, argv);

  if (result.count("help") > 0 || result.count("scene") == 0 ||
      result.count("output") == 0) {
    std::cerr << options.help() << '\n';
    return EXIT_FAILURE;
  }

  auto scene_file = result["scene"].as<std::string>();
  auto output_file = result["output"].as<std::string>();
  auto size = parse_dims(result["dims"].as<std::string>());

  auto fs = cmrc::shaders::get_filesystem();

  auto f = fs.open("compute.wgsl");
  std::string source{f.begin(), f.end()};
  Renderer renderer{source};
  auto props = renderer.adapter_properties();
  std::cerr << "GPU: " << props.name << '\n';

  Scene scene = load_scene(scene_file);
  auto output = renderer.render_scene(std::move(scene), size);

  stbi_write_png(output_file.c_str(), size.x, size.y, 4,
                 output.data(), size.x * 4);

  return EXIT_SUCCESS;
}
