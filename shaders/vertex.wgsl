struct VSOut {
  @builtin(position) pos: vec4<f32>,
  @location(0) coord: vec2<f32>
};

@vertex
fn main(@builtin(vertex_index) idx : u32) -> VSOut {
  var data = array<vec2<f32>, 6>(
    vec2<f32>(-1.0, -1.0),
    vec2<f32>(1.0, -1.0),
    vec2<f32>(1.0, 1.0),

    vec2<f32>(-1.0, -1.0),
    vec2<f32>(-1.0, 1.0),
    vec2<f32>(1.0, 1.0),
  );

  var pos = data[idx];

  var out : VSOut;
  out.pos = vec4<f32>(pos, 0.0, 1.0);
  out.coord.x = (pos.x + 1.0) / 2.0;
  out.coord.y = (1.0 - pos.y) / 2.0;

  return out;
}
