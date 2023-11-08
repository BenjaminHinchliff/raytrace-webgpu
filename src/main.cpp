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
    ("s,scene", "Scene input file", cxxopts::value<std::string>())
    ("o,output", "Output file", cxxopts::value<std::string>())
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

  auto fs = cmrc::shaders::get_filesystem();

  auto f = fs.open("compute.wgsl");
  std::string source{f.begin(), f.end()};
  Renderer renderer{source};
  auto props = renderer.adapter_properties();
  std::cerr << "GPU: " << props.name << '\n';

  TracerConfig config{
      .size = {640, 480},
  };
  Scene scene = load_scene(scene_file);
  auto output = renderer.render_scene(std::move(scene), config.size);

  stbi_write_png(output_file.c_str(), config.size.x, config.size.y, 4,
                 output.data(), config.size.x * 4);

  return EXIT_SUCCESS;
}
