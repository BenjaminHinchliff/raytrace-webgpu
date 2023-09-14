#ifndef MATERIALS_METAL_HPP_
#define MATERIALS_METAL_HPP_

#include "material.hpp"

#include <glm/vec3.hpp>

class Metal : public Material {
public:
  Metal(glm::vec3 albedo, float fuzz);

  std::string generate() const override;

private:
  glm::vec3 albedo;
  float fuzz;
};

#endif // !MATERIALS_METAL_HPP_
