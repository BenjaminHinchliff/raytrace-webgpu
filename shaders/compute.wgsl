@group(0) @binding(0)
var output_texture: texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dims = textureDimensions(output_texture);
    let coords = vec2<u32>(global_id.xy);
    if coords.x >= dims.x || coords.y >= dims.y {
        return;
    }

    let color = vec3<f32>(vec2<f32>(coords) / vec2<f32>(dims), 0.0);
    textureStore(output_texture, coords.xy, vec4<f32>(color, 1.0));
}
