#ifndef PLANE_HPP_
#define PLANE_HPP_

#include "hittable.hpp"

#include <glm/vec3.hpp>

class Plane : public Hittable {
public:
  Plane(glm::vec3 point, glm::vec3 normal);

  virtual std::string generate() const override;

private:
  glm::vec3 point;
  glm::vec3 normal;
};

#endif // !PLANE_HPP_
