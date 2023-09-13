#include "scene.hpp"

#include <string_view>

Scene::Scene(std::vector<std::unique_ptr<Hittable>> hittables)
    : hittables{std::move(hittables)} {}

// clang-format off
constexpr std::string_view GENERATION_HEADER = CODE(
  fn hit_scene(ray: Ray, tmin: f32, tmax: f32) -> HitRecord {
    var record: HitRecord;
    record.hit = false;
    record.t = tmax;
    var temp_rec: HitRecord;
);

constexpr std::string_view GENERATION_FOOTER = CODE(
    return record;
  }
);
// clang-format on

std::string Scene::generate() const {
  std::string body{GENERATION_HEADER};
  for (const auto &hittable : hittables) {
    body += hittable->generate();
  }

  return body + std::string{GENERATION_FOOTER};
}
