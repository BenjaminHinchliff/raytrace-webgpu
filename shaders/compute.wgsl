@group(0) @binding(0)
var output_texture: texture_storage_2d<rgba8unorm, write>;

struct Ray {
    origin: vec3<f32>,
    direction: vec3<f32>,
}

fn hit_sphere(center: vec3<f32>, radius: f32, ray: Ray) -> bool {
    let oc = ray.origin - center;
    let a = dot(ray.direction, ray.direction);
    let b = 2.0 * dot(oc, ray.direction);
    let c = dot(oc, oc) - radius * radius;
    let discriminant = b * b - 4 * a * c;
    return discriminant >= 0;
}

fn ray_color(ray: Ray) -> vec3<f32> {
    if hit_sphere(vec3<f32>(0.0, -0.5, -1.0), 0.5, ray) {
        return vec3<f32>(1.0, 0.0, 0.0);
    }

    let unit_dir = normalize(ray.direction);
    let a = 0.5 * (unit_dir.y + 1.0);
    return (1.0 - a) * vec3<f32>(1.0, 1.0, 1.0) + a * vec3<f32>(0.5, 0.7, 1.0);
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dims = textureDimensions(output_texture);
    let coords = vec2<u32>(global_id.xy);
    if coords.x >= dims.x || coords.y >= dims.y {
        return;
    }

    let aspect = f32(dims.y) / f32(dims.x);
    let focal_length = 1.0;
    let viewport_height = 2.0;
    let viewport = vec2<f32>(viewport_height / aspect, viewport_height);
    let viewport_delta = vec2<f32>(viewport.x / f32(dims.x), -viewport.y / f32(dims.y));
    let viewport_upper_left = vec3<f32>(vec2<f32>(-viewport.x / 2.0, viewport.y / 2.0) + 0.5 * viewport_delta, -focal_length);
    let uv = viewport_upper_left + vec3<f32>(viewport_delta * vec2<f32>(coords.xy), 0.0);
    let ray = Ray(vec3<f32>(0.0), uv);
    let color = ray_color(ray);
    textureStore(output_texture, coords.xy, vec4<f32>(color, 1.0));
}
