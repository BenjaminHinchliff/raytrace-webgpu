@fragment
fn main(@location(0) coord: vec2<f32>) -> @location(0) vec4<f32> {
  return vec4<f32>(coord.x, coord.y, 0.5, 1.0);
}
