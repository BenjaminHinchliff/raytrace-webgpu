#ifndef HITTABLE_HPP_
#define HITTABLE_HPP_

#include "materials/material.hpp"

#include <format>
#include <memory>
#include <string>
#include <string_view>

class Hittable {
public:
  Hittable(std::shared_ptr<Material> material);
  virtual ~Hittable();

  virtual std::string generate() const = 0;

protected:
  std::string place_in_template(std::string_view postfix,
                                std::string_view builder) const;

private:
  std::shared_ptr<Material> material;
};

#endif // !HITTABLE_HPP_
