#ifndef MATERIALS_DIELECTRIC_HPP_
#define MATERIALS_DIELECTRIC_HPP_

#include "material.hpp"

class Dielectric : public Material {
public:
  Dielectric(float ir);

  std::string generate() const override;

private:
  float ir;
};

#endif // !MATERIALS_DIELECTRIC_HPP_
