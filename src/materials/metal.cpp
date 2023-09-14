#include "metal.hpp"
#include "code.hpp"

#include <format>

Metal::Metal(glm::vec3 albedo, float fuzz) : albedo{albedo}, fuzz{fuzz} {}

std::string Metal::generate() const {
  // clang-format off
  return std::format(CODE(
    record.material = Material(MATERIAL_METAL, vec4<f32>({}, {}, {}, {}));
  ), albedo.x, albedo.y, albedo.z, fuzz);
  // clang-format on
}
