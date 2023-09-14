#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <string>

class Material {
public:
  Material();
  virtual ~Material();

  virtual std::string generate() const = 0;
};

#endif // !MATERIAL_H_
