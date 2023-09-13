#ifndef HITTABLE_HPP_
#define HITTABLE_HPP_

#include <format>
#include <string>
#include <string_view>

#define CODE(...) #__VA_ARGS__

class Hittable {
public:
  Hittable(){};
  virtual ~Hittable(){};

  virtual std::string generate() const = 0;

protected:
  std::string place_in_template(std::string_view postfix,
                                std::string_view builder) const;
};

#endif // !HITTABLE_HPP_
