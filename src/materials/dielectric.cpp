#include "dielectric.hpp"
#include "code.hpp"

#include <format>

Dielectric::Dielectric(float ir) : ir{ir} {}

std::string Dielectric::generate() const {
  // clang-format off
  return std::format(CODE(
    record.material = Material(MATERIAL_DIELECTRIC, vec4<f32>(1.0, 1.0, 1.0, {}));
  ), ir);
  //clang-format on
}
