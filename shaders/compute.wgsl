@group(0) @binding(0)
var output_texture: texture_storage_2d<rgba8unorm, write>;

struct Sphere {
    center: vec3<f32>,
    radius: f32,
}

struct Ray {
    origin: vec3<f32>,
    direction: vec3<f32>,
}

fn ray_at(ray: Ray, t: f32) -> vec3<f32> {
    return ray.origin + ray.direction * t;
}

struct HitRecord {
    hit: bool,
    t: f32,
    normal: vec3<f32>,
    front_face: bool,
}

fn hitrecord_set_face_normal(record: ptr<function, HitRecord>, ray: Ray) {
    (*record).front_face = dot(ray.direction, (*record).normal) < 0.0;
    if (*record).front_face {
        (*record).normal = (*record).normal;
    } else {
        (*record).normal = -(*record).normal;
    }
}

// , record: HitRecord
fn hit(sphere: Sphere, ray: Ray, tmin: f32, tmax: f32) -> HitRecord {
    var record: HitRecord;
    record.hit = false;

    let oc = ray.origin - sphere.center;
    let a = dot(ray.direction, ray.direction);
    let half_b = dot(oc, ray.direction);
    let c = dot(oc, oc) - sphere.radius * sphere.radius;

    let discriminant = half_b * half_b - a * c;
    if discriminant < 0.0 {
        return record;
    }

    let sqrtd = sqrt(discriminant);

    var root = (-half_b - sqrtd) / a;
    if root <= tmin || root >= tmax {
        root = (-half_b + sqrtd) / a;
        if root <= tmin || root >= tmax {
            return record;
        }
    }

    record.hit = true;
    record.t = root;
    record.normal = (ray_at(ray, record.t) - sphere.center) / sphere.radius;
    hitrecord_set_face_normal(&record, ray);
    return record;
}

const RAY_MAX: f32 = 1e30;

fn ray_color(ray: Ray) -> vec3<f32> {
    let sphere = Sphere(vec3<f32>(0.0, 0.0, -1.0), 0.5);
    let record = hit(sphere, ray, 0.0, RAY_MAX);
    if record.hit {
        return 0.5 * (record.normal + vec3<f32>(1.0));
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
