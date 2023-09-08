set(SHADERS
  "fragment.wgsl"
  "vertex.wgsl")

foreach(SHADER ${SHADERS})
  configure_file(${SHADER} ${CMAKE_CURRENT_BINARY_DIR}/${SHADER} COPYONLY)
endforeach()
