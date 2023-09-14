#ifndef MATERIALS_MATERIAL_H_
#define MATERIALS_MATERIAL_H_

#include <string>

class Material {
public:
  Material();
  virtual ~Material();

  virtual std::string generate() const = 0;
};

#endif // !MATERIALS_MATERIAL_H_
