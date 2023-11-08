#include "load.hpp"
#include "hittables/hittable.hpp"
#include "hittables/plane.hpp"
#include "hittables/sphere.hpp"
#include "materials/dielectric.hpp"
#include "materials/lambertian.hpp"
#include "materials/material.hpp"
#include "materials/metal.hpp"

#include <yaml-cpp/yaml.h>
#include <glm/ext/vector_float3.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#define SCENE_ASSERT(condition, message)                                       \
  if (!(condition)) {                                                          \
    throw std::runtime_error{message};                                         \
  }

glm::vec3 load_vec3(YAML::Node seq) {
  SCENE_ASSERT(seq.IsSequence(), "Vec3 must be of type sequence");
  SCENE_ASSERT(seq.size() == 3, "Vec3 must be of length 3");

  auto seq_iter = seq.begin();
  float x = seq_iter->as<float>();
  ++seq_iter;
  float y = seq_iter->as<float>();
  ++seq_iter;
  float z = seq_iter->as<float>();
  ++seq_iter;
  return glm::vec3{x, y, z};
}

Scene load_scene(const std::string &path) {
  using namespace std::string_literals;
  YAML::Node yaml = YAML::LoadFile(path);
  SCENE_ASSERT(yaml.IsMap(), "Expected scene root type to be map");

  std::unordered_map<std::string, std::shared_ptr<Material>> materials;
  for (const auto &node : yaml["materials"]) {
    SCENE_ASSERT(node.size() == 1, "Expected hittable to have only one key");
    auto material = *node.begin();
    auto name = material.first.as<std::string>();
    SCENE_ASSERT(material.second.size() == 1,
                 "Expected material type to have only one key");
    auto inner = *material.second.begin();
    auto type = inner.first.as<std::string>();
    auto body = inner.second;
    if (type == "lambertian") {
      materials[name] = std::make_shared<Lambertian>(load_vec3(body["albedo"]));
    } else if (type == "dielectric") {
      materials[name] = std::make_shared<Dielectric>(body["ir"].as<float>());
    } else if (type == "metal") {
      materials[name] = std::make_shared<Metal>(load_vec3(body["albedo"]),
                                                body["fuzz"].as<float>());
    } else {
      throw std::runtime_error{"unkown material type!"};
    }
  }

  auto red = std::make_shared<Lambertian>(glm::vec3{0.7, 0.0, 0.0});
  std::vector<std::unique_ptr<Hittable>> hittables;
  for (const auto &node : yaml["hittables"]) {
    SCENE_ASSERT(node.size() == 1, "Expected hittable to have only one key");
    auto hittable = *node.begin();
    auto type = hittable.first.as<std::string>();
    auto body = hittable.second;
    auto material = materials[body["material"].as<std::string>()];
    if (type == "sphere") {
      hittables.push_back(std::make_unique<Sphere>(
          load_vec3(body["center"]), body["radius"].as<float>(), material));
    } else if (type == "plane") {
      hittables.push_back(std::make_unique<Plane>(
          load_vec3(body["point"]), load_vec3(body["normal"]), material));
    } else {
      throw std::runtime_error{"unkown object type!"};
    }
  }

  Scene scene{std::move(hittables)};
  return scene;
}
