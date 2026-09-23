#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>

namespace ax {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTau = 6.28318530717958647692f;
constexpr float kDeg = kPi / 180.0f;
constexpr float kEps = 1e-6f;

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(Vec2 b) const { return {x + b.x, y + b.y}; }
    Vec2 operator-(Vec2 b) const { return {x - b.x, y - b.y}; }
    Vec2 operator-() const { return {-x, -y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(Vec2 b) { x += b.x; y += b.y; return *this; }
    float dot(Vec2 b) const { return x * b.x + y * b.y; }
    float cross(Vec2 b) const { return x * b.y - y * b.x; }
    float length2() const { return x * x + y * y; }
    float length() const { return std::sqrt(length2()); }
    Vec2 normalized() const {
        float l = length();
        return l > kEps ? (*this) / l : Vec2{};
    }
};

inline Vec2 operator*(float s, Vec2 v) { return v * s; }

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    explicit Vec3(float s) : x(s), y(s), z(s) {}
    Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3 operator*(Vec3 b) const { return {x * b.x, y * b.y, z * b.z}; }
    Vec3& operator+=(Vec3 b) { x += b.x; y += b.y; z += b.z; return *this; }
    Vec3& operator-=(Vec3 b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
    float dot(Vec3 b) const { return x * b.x + y * b.y + z * b.z; }
    Vec3 cross(Vec3 b) const {
        return {y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x};
    }
    float length2() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(length2()); }
    Vec3 normalized() const {
        float l = length();
        return l > kEps ? (*this) / l : Vec3{0, 0, 1};
    }
    Vec3 abs() const { return {std::fabs(x), std::fabs(y), std::fabs(z)}; }
    float max_comp() const { return std::max(x, std::max(y, z)); }
    float min_comp() const { return std::min(x, std::min(y, z)); }
};

inline Vec3 operator*(float s, Vec3 v) { return v * s; }
inline Vec3 minv(Vec3 a, Vec3 b) { return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; }
inline Vec3 maxv(Vec3 a, Vec3 b) { return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; }
inline Vec3 lerp(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }
inline float clamp(float v, float a, float b) { return std::max(a, std::min(b, v)); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float saturate(float v) { return clamp(v, 0.f, 1.f); }

struct Vec4 {
    float x = 0, y = 0, z = 0, w = 0;
    Vec4() = default;
    constexpr Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    Vec4(Vec3 v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
    Vec3 xyz() const { return {x, y, z}; }
};

struct Mat4 {
    float m[16]{}; // column-major

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
        return r;
    }
    static Mat4 translate(Vec3 t) {
        Mat4 r = identity();
        r.m[12] = t.x;
        r.m[13] = t.y;
        r.m[14] = t.z;
        return r;
    }
    static Mat4 scale(Vec3 s) {
        Mat4 r{};
        r.m[0] = s.x;
        r.m[5] = s.y;
        r.m[10] = s.z;
        r.m[15] = 1;
        return r;
    }
    static Mat4 rotate_x(float a) {
        float c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.m[5] = c;
        r.m[6] = s;
        r.m[9] = -s;
        r.m[10] = c;
        return r;
    }
    static Mat4 rotate_y(float a) {
        float c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.m[0] = c;
        r.m[2] = -s;
        r.m[8] = s;
        r.m[10] = c;
        return r;
    }
    static Mat4 rotate_z(float a) {
        float c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.m[0] = c;
        r.m[1] = s;
        r.m[4] = -s;
        r.m[5] = c;
        return r;
    }
    // Z-up CAD Euler: yaw about Z, pitch about Y, roll about X — applied as Rz * Ry * Rx
    static Mat4 euler_zyx(Vec3 deg) {
        return rotate_z(deg.z * kDeg) * rotate_y(deg.y * kDeg) * rotate_x(deg.x * kDeg);
    }
    static Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
        Vec3 f = (target - eye).normalized();
        Vec3 r = f.cross(up).normalized();
        if (r.length2() < 1e-10f) {
            up = {0, 0, 1};
            r = f.cross(up).normalized();
            if (r.length2() < 1e-10f) {
                up = {0, 1, 0};
                r = f.cross(up).normalized();
            }
        }
        Vec3 u = r.cross(f);
        Mat4 m = identity();
        m.m[0] = r.x;
        m.m[4] = r.y;
        m.m[8] = r.z;
        m.m[1] = u.x;
        m.m[5] = u.y;
        m.m[9] = u.z;
        m.m[2] = -f.x;
        m.m[6] = -f.y;
        m.m[10] = -f.z;
        m.m[12] = -r.dot(eye);
        m.m[13] = -u.dot(eye);
        m.m[14] = f.dot(eye);
        return m;
    }
    static Mat4 perspective(float fovy_deg, float aspect, float zn, float zf) {
        float f = 1.f / std::tan(fovy_deg * kDeg * 0.5f);
        Mat4 r{};
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (zf + zn) / (zn - zf);
        r.m[11] = -1;
        r.m[14] = (2 * zf * zn) / (zn - zf);
        return r;
    }
    static Mat4 ortho(float l, float rgt, float b, float t, float zn, float zf) {
        Mat4 r{};
        r.m[0] = 2 / (rgt - l);
        r.m[5] = 2 / (t - b);
        r.m[10] = -2 / (zf - zn);
        r.m[12] = -(rgt + l) / (rgt - l);
        r.m[13] = -(t + b) / (t - b);
        r.m[14] = -(zf + zn) / (zf - zn);
        r.m[15] = 1;
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r{};
        for (int c = 0; c < 4; ++c) {
            for (int row = 0; row < 4; ++row) {
                r.m[c * 4 + row] =
                    m[0 * 4 + row] * b.m[c * 4 + 0] +
                    m[1 * 4 + row] * b.m[c * 4 + 1] +
                    m[2 * 4 + row] * b.m[c * 4 + 2] +
                    m[3 * 4 + row] * b.m[c * 4 + 3];
            }
        }
        return r;
    }
    Vec4 mul(Vec4 v) const {
        return {
            m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
            m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
            m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
            m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w};
    }
    Vec3 transform_point(Vec3 p) const {
        Vec4 r = mul(Vec4(p, 1));
        return r.w != 0 ? r.xyz() / r.w : r.xyz();
    }
    Vec3 transform_vec(Vec3 v) const { return mul(Vec4(v, 0)).xyz(); }
    Mat4 inverse() const;
};

inline Mat4 Mat4::inverse() const {
    const float* a = m;
    float inv[16];
    inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] +
             a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
    inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] -
             a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
    inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] +
             a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
    inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] -
              a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
    inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] -
             a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
    inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] +
             a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
    inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] -
             a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
    inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] +
              a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
    inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] +
             a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
    inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] -
             a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
    inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] +
              a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
    inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] -
              a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
    inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] -
             a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
    inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] +
             a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
    inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] -
              a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
    inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] +
              a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];
    float det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
    Mat4 out{};
    if (std::fabs(det) < 1e-12f) return identity();
    det = 1.f / det;
    for (int i = 0; i < 16; ++i) out.m[i] = inv[i] * det;
    return out;
}

struct Ray {
    Vec3 o, d;
    Vec3 at(float t) const { return o + d * t; }
};

struct Plane {
    Vec3 n{0, 0, 1};
    float d = 0; // n·x = d
    static Plane from_point_normal(Vec3 p, Vec3 normal) {
        Vec3 nn = normal.normalized();
        return {nn, nn.dot(p)};
    }
    float signed_dist(Vec3 p) const { return n.dot(p) - d; }
};

struct Aabb {
    Vec3 mn{1e9f, 1e9f, 1e9f};
    Vec3 mx{-1e9f, -1e9f, -1e9f};
    bool valid() const { return mx.x >= mn.x; }
    void expand(Vec3 p) {
        mn = minv(mn, p);
        mx = maxv(mx, p);
    }
    void expand(const Aabb& b) {
        if (!b.valid()) return;
        expand(b.mn);
        expand(b.mx);
    }
    Vec3 center() const { return (mn + mx) * 0.5f; }
    Vec3 extent() const { return (mx - mn) * 0.5f; }
    Vec3 size() const { return mx - mn; }
    float max_extent() const { return size().max_comp(); }
    Aabb padded(float p) const {
        Aabb r = *this;
        r.mn = mn - Vec3{p, p, p};
        r.mx = mx + Vec3{p, p, p};
        return r;
    }
    bool overlaps(const Aabb& o, float shrink = 0) const {
        if (!valid() || !o.valid()) return false;
        return mn.x + shrink < o.mx.x - shrink && mx.x - shrink > o.mn.x + shrink &&
               mn.y + shrink < o.mx.y - shrink && mx.y - shrink > o.mn.y + shrink &&
               mn.z + shrink < o.mx.z - shrink && mx.z - shrink > o.mn.z + shrink;
    }
    float volume() const {
        if (!valid()) return 0;
        Vec3 s = size();
        return s.x * s.y * s.z;
    }
};

inline bool intersect_ray_plane(const Ray& ray, const Plane& pl, float& t, Vec3& hit) {
    float denom = pl.n.dot(ray.d);
    if (std::fabs(denom) < 1e-8f) return false;
    t = (pl.d - pl.n.dot(ray.o)) / denom;
    if (t < 0) return false;
    hit = ray.at(t);
    return true;
}

inline bool intersect_ray_tri(const Ray& ray, Vec3 a, Vec3 b, Vec3 c, float& t, float& u, float& v) {
    Vec3 e1 = b - a, e2 = c - a;
    Vec3 p = ray.d.cross(e2);
    float det = e1.dot(p);
    if (std::fabs(det) < 1e-10f) return false;
    float inv = 1.f / det;
    Vec3 s = ray.o - a;
    u = s.dot(p) * inv;
    if (u < 0 || u > 1) return false;
    Vec3 q = s.cross(e1);
    v = ray.d.dot(q) * inv;
    if (v < 0 || u + v > 1) return false;
    t = e2.dot(q) * inv;
    return t > 1e-5f;
}

inline Vec2 closest_point_seg(Vec2 p, Vec2 a, Vec2 b, float& t) {
    Vec2 ab = b - a;
    float l2 = ab.length2();
    if (l2 < kEps) {
        t = 0;
        return a;
    }
    t = clamp((p - a).dot(ab) / l2, 0.f, 1.f);
    return a + ab * t;
}

} // namespace ax
