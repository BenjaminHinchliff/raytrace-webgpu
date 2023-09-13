#include "sphere.hpp"

#include <format>
#include <string_view>

Sphere::Sphere(glm::vec3 center, float radius)
    : center{center}, radius{radius} {}

std::string Sphere::generate() const {
  auto builder = std::format("Sphere(vec3<f32>({}, {}, {}), {})", center.x,
                             center.y, center.z, radius);
  return place_in_template("sphere", builder);
}
