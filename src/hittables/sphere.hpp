#ifndef HITTABLES_SPHERE_H_
#define HITTABLES_SPHERE_H_

#include "hittable.hpp"

#include <glm/vec3.hpp>

#include <string>

class Sphere : public Hittable {
public:
  Sphere(glm::vec3 center, float radius, std::shared_ptr<Material> material);

  virtual std::string generate() const override;

private:
  glm::vec3 center;
  float radius;
};

#endif // !HITTABLES_SPHERE_H_
