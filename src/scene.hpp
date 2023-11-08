#ifndef SCENE_H_
#define SCENE_H_

#include "hittables/hittable.hpp"

#include <memory>
#include <string>
#include <vector>

class Scene {
public:
  Scene();
  Scene(std::vector<std::unique_ptr<Hittable>> hittables);

  std::string generate() const;

private:
  std::vector<std::unique_ptr<Hittable>> hittables;
};

#endif // !SCENE_H_
