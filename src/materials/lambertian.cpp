#include "lambertian.hpp"
#include "code.hpp"

#include <format>

Lambertian::Lambertian(glm::vec3 albedo) : albedo{albedo} {}

std::string Lambertian::generate() const {
  // clang-format off
  return std::format(CODE(
    record.material = Material(MATERIAL_LAMBERTIAN, vec4<f32>({}, {}, {}, 0.0));
  ), albedo.x, albedo.y, albedo.z);
  // clang-format on
}
