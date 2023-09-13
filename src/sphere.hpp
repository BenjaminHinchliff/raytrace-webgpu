#ifndef SPHERE_H_
#define SPHERE_H_

#include "hittable.hpp"

#include <glm/vec3.hpp>

#include <string>

class Sphere : public Hittable {
public:
  Sphere(glm::vec3 center, float radius);

  virtual std::string generate() const override;

private:
  glm::vec3 center;
  float radius;
};

#endif // !SPHERE_H_
