#ifndef MATERIALS_LAMBERTIAN_HPP_
#define MATERIALS_LAMBERTIAN_HPP_

#include "material.hpp"

#include <glm/vec3.hpp>

class Lambertian : public Material {
public:
  Lambertian(glm::vec3 albedo);

  std::string generate() const override;

private:
  glm::vec3 albedo;
};

#endif // !MATERIALS_LAMBERTIAN_HPP_
