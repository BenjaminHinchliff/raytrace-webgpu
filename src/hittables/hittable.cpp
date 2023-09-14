#include "hittable.hpp"

#include "code.hpp"

#include <format>
#include <memory>
#include <string>
#include <string_view>

Hittable::Hittable(std::shared_ptr<Material> material) : material{material} {}

Hittable::~Hittable() {}

std::string Hittable::place_in_template(std::string_view postfix,
                                        std::string_view builder) const {
  // clang-format off
  return std::format(CODE(
      temp_rec = hit_{}({}, ray, tmin, record.t);
      if temp_rec.hit {{
          record = temp_rec;
          {}
      }}
  ), postfix, builder, material->generate());
  // clang-format on
  }
