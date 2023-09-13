@group(0) @binding(0)
var output_texture: texture_storage_2d<rgba8unorm, write>;

// xorshift rng
var<private> s: u32;
// seed with linear prng
fn seed(id: u32) {
    s = id * 1099087573;
    // run one cycle so the first random number out isn't from lcg
    xorshift32();
}

// super low quality, check if causes visual artifacts
fn xorshift32() -> u32 {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

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
    point: vec3<f32>,
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
fn hit_sphere(sphere: Sphere, ray: Ray, tmin: f32, tmax: f32) -> HitRecord {
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
    record.point = ray_at(ray, record.t);
    record.normal = (record.point - sphere.center) / sphere.radius;
    hitrecord_set_face_normal(&record, ray);
    return record;
}

struct Plane {
    point: vec3<f32>,
    normal: vec3<f32>,
}

fn hit_plane(plane: Plane, ray: Ray, tmin: f32, tmax: f32) -> HitRecord {
    var record: HitRecord;
    record.hit = false;

    let denom = dot(plane.normal, ray.direction);
    if denom > 1e-6 {
        let t = dot(plane.point - ray.origin, plane.normal) / denom;
        if t <= tmin || t >= tmax {
            return record;
        }
        record.hit = true;
        record.t = t;
        record.point = ray_at(ray, record.t);
        record.normal = plane.normal;
        hitrecord_set_face_normal(&record, ray);
        return record;
    }

    return record;
}

const U32_MAX: u32 = 4294967295;

fn random_f32() -> f32 {
    return f32(xorshift32()) / f32(U32_MAX);
}

fn random_f32_range(min: f32, max: f32) -> f32 {
    return min + (max - min) * random_f32();
}

fn random_vec3() -> vec3<f32> {
    return vec3<f32>(random_f32_range(-1.0, 1.0), random_f32_range(-1.0, 1.0), random_f32_range(-1.0, 1.0));
}

fn random_vec3_in_unit_sphere() -> vec3<f32> {
    while true {
        let p = random_vec3();
        if dot(p, p) < 1.0 {
            return p;
        }
    }
    return vec3<f32>();
}

fn random_vec3_normalized() -> vec3<f32> {
    return normalize(random_vec3_in_unit_sphere());
}

const RAY_MAX: f32 = 1e30;
const MAX_DEPTH: i32 = 10;
fn ray_color(ray: Ray) -> vec3<f32> {
    let unit_dir = normalize(ray.direction);
    let a = 0.5 * (unit_dir.y + 1.0);
    var color = (1.0 - a) * vec3<f32>(1.0, 1.0, 1.0) + a * vec3<f32>(0.5, 0.7, 1.0);
    var cur_ray = ray;
    var record = hit_scene(cur_ray, 0.001, RAY_MAX);
    for (var i = 0; record.hit && i < MAX_DEPTH; i++) {
        color = 0.5 * color;
        let direction = record.normal + random_vec3_normalized();
        cur_ray = Ray(record.point, direction);
        record = hit_scene(cur_ray, 0.001, RAY_MAX);
    }

    return color;
}

const SAMPLES_PER_PIXEL = 100;

fn random_uv_noise() -> vec2<f32> {
    return vec2<f32>(random_f32_range(-0.5, 0.5), random_f32_range(-0.5, 0.5));
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let dims = textureDimensions(output_texture);
    let coords = vec2<u32>(global_id.xy);

    // check out of bounds on texture
    if coords.x >= dims.x || coords.y >= dims.y {
        return;
    }

    // seed rng
    // unique id of the thread
    let uid = dims.x * coords.y + coords.x;
    seed(uid);

    let aspect = f32(dims.y) / f32(dims.x);
    let focal_length = 0.5;
    let viewport_height = 2.0;
    let viewport = vec2<f32>(viewport_height / aspect, viewport_height);
    let viewport_delta = vec2<f32>(viewport.x / f32(dims.x), -viewport.y / f32(dims.y));
    let viewport_upper_left = vec3<f32>(vec2<f32>(-viewport.x / 2.0, viewport.y / 2.0) + 0.5 * viewport_delta, -focal_length);
    let uv = viewport_upper_left + vec3<f32>(viewport_delta * vec2<f32>(coords.xy), 0.0);

    var color = vec3<f32>(0.0);
    for (var i = 0; i < SAMPLES_PER_PIXEL; i++) {
        let noise = vec3<f32>(random_uv_noise() * viewport_delta, 0.0);
        let ray = Ray(vec3<f32>(0.0), uv + noise);
        color += ray_color(ray);
    }

    textureStore(output_texture, coords.xy, vec4<f32>(color / SAMPLES_PER_PIXEL, 1.0));
}
