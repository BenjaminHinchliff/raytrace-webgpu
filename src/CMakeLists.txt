set(TRACEG_INC
    "render.hpp"
    "scene.hpp"
    "hittables/hittable.hpp"
    "hittables/sphere.hpp"
    "hittables/plane.hpp"
    "materials/material.hpp"
    "materials/lambertian.hpp"
    "materials/metal.hpp"
    "materials/dielectric.hpp")

set(TRACEG_SRC
    "main.cpp"
    "render.cpp"
    "scene.cpp"
    "stb-impl.cpp"
    "hittables/hittable.cpp"
    "hittables/sphere.cpp"
    "hittables/plane.cpp"
    "materials/material.cpp"
    "materials/lambertian.cpp"
    "materials/metal.cpp"
    "materials/dielectric.cpp")

add_executable(traceg ${TRACEG_SRC} ${TRACEG_INC})
target_include_directories(traceg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(traceg PRIVATE cxx_std_20)
target_link_libraries(traceg PRIVATE webgpu_cpp webgpu_dawn stb glm shaders
                                     cxxopts)

if(MSVC)
  target_compile_options(traceg PRIVATE /W4 /WX)
else()
  target_compile_options(traceg PRIVATE -Wall -Wextra -Wpedantic)
endif()
