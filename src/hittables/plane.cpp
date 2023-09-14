#include "plane.hpp"
#include "hittables/hittable.hpp"

Plane::Plane(glm::vec3 point, glm::vec3 normal,
             std::shared_ptr<Material> material)
    : Hittable(material), point{point}, normal{normal} {}

std::string Plane::generate() const {
  auto builder =
      std::format("Plane(vec3<f32>({}, {}, {}), vec3<f32>({}, {}, {}))",
                  point.x, point.y, point.z, normal.x, normal.y, normal.z);
  return place_in_template("plane", builder);
}
