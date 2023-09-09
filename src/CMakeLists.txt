set(TRACEG_INC)

set(TRACEG_SRC
  "main.cpp")

add_executable(traceg ${TRACEG_SRC} ${TRACEG_INC})

target_compile_features(traceg PRIVATE cxx_std_20)
target_link_libraries(traceg PRIVATE webgpu_dawn webgpu_glfw webgpu_cpp GLFWPP)

if(MSVC)
  target_compile_options(traceg PRIVATE /W4 /WX)
else()
  target_compile_options(traceg PRIVATE -Wall -Wextra -Wpedantic)
endif()
