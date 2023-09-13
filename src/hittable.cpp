#include "hittable.hpp"

#include <format>
#include <string>
#include <string_view>

std::string Hittable::place_in_template(std::string_view postfix,
                                        std::string_view builder) const {
  // clang-format off
  return std::format(CODE(
      temp_rec = hit_{}({}, ray, tmin, record.t);
      if temp_rec.hit {{
          record = temp_rec;
      }}
  ), postfix, builder);
  // clang-format on
  }
