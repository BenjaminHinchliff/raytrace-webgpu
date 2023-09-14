set(TRACEG_INC
  "render.hpp"
  "scene.hpp"
  "hittables/hittable.hpp"
  "hittables/sphere.hpp"
  "hittables/plane.hpp"
  "materials/material.hpp"
  "materials/lambertian.hpp")

set(TRACEG_SRC
  "main.cpp"
  "render.cpp"
  "scene.cpp"
  "hittables/hittable.cpp"
  "hittables/sphere.cpp"
  "hittables/plane.cpp"
  "materials/material.cpp"
  "materials/lambertian.cpp")

add_executable(traceg ${TRACEG_SRC} ${TRACEG_INC})
target_include_directories(traceg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(traceg PRIVATE cxx_std_20)
target_link_libraries(traceg PRIVATE webgpu_dawn webgpu_cpp stb glm shaders)

if(MSVC)
  target_compile_options(traceg PRIVATE /W4 /WX)
else()
  target_compile_options(traceg PRIVATE -Wall -Wextra -Wpedantic)
endif()
