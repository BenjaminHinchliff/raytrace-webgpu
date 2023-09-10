@group(0) @binding(0)
var output_texture: texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dims = textureDimensions(output_texture);
    let coords = vec2<u32>(global_id.xy);
    if coords.x >= dims.x || coords.y >= dims.y {
        return;
    }

    let aspect = f32(dims.y) / f32(dims.x);

    let viewport_height = 2.0;
    let viewport = vec2<f32>(viewport_height / aspect, viewport_height);
    let viewport_delta = viewport / vec2<f32>(dims);
    let viewport_upper_left = -viewport / 2.0;
    let uv = viewport_upper_left + viewport_delta * vec2<f32>(coords.xy);
    let color = vec3<f32>(abs(uv), 0.0);
    textureStore(output_texture, coords.xy, vec4<f32>(color, 1.0));
}
