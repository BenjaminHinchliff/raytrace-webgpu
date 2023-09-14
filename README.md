# WebGPU Raytracer

A WebGPU Raytracer chromium's Dawn WebGPU implementation. Supports creation of
scenes program side (see `src/main.cpp`), along with multiple primitives
(sphere, plane) and materials (lambertian, metal, dielectric) using dynamically
generated wgsl shaders.

## Building and Running

Dependencies are managed using `CPM`, so building can be done with a simple
clone and cmake setup. All dependencies are pulled and built from source at
present (though in the future GLFW may be fixed in that respect), so initial
build can take quite a while. Further builds can be sped up using a caching
compiler (e.g. ccache) and dependency caching (see CPM repo).

```shell
git clone https://github.com/BenjaminHinchliff/raytrace-webgpu.git

# download dependencies and build
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -Bbuild .
```
