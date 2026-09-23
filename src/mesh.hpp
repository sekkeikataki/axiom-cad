#pragma once

#include "math.hpp"
#include <functional>
#include <string>
#include <vector>

namespace ax {

struct Vertex {
    Vec3 p;
    Vec3 n;
    std::uint32_t feature_id = 0;
    float scalar = 0; // analysis / quality paint, 0..1
};

struct Hit {
    bool hit = false;
    float t = 1e9f;
    Vec3 p;
    Vec3 n;
    std::uint32_t feature_id = 0;
    std::uint32_t tri = 0;
};

struct HalfEdge {
    std::uint32_t orig = 0;
    std::uint32_t dest = 0;
    std::int32_t next = -1;
    std::int32_t twin = -1;
    std::int32_t face = -1;
};

struct CreaseId {
    bool ok = false;
    bool loop = false;
    std::uint64_t fa = 0;
    std::uint64_t fb = 0;
    int slot = 0;
    Vec3 n1{0, 0, 1};
    Vec3 n2{0, 1, 0};
};

Vec3 snap_face_normal(Vec3 n);
std::uint64_t face_key(int feature_id, Vec3 n);

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Vec3> edges; // pairs of points for crease/silhouette overlay
    Aabb bounds;

    std::vector<HalfEdge> halfs;
    std::vector<std::int32_t> vout;
    std::vector<std::int32_t> onext;
    std::vector<std::uint32_t> weld;
    std::vector<Vec3> wpos;
    std::vector<std::uint8_t> he_crease;
    bool topo_ok = false;

    void clear();
    void invalidate_topo();
    void add_tri(Vec3 a, Vec3 b, Vec3 c, std::uint32_t fid);
    void add_quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, std::uint32_t fid);
    void compute_bounds();
    void compute_smooth_normals();
    void extract_crease_edges(float degrees = 38.f);
    void build_topology(float crease_deg = 38.f);
    std::vector<Vec3> crease_chain(Vec3 a, Vec3 b, Vec3 prefer_n = {}, int max_seg = 48, bool* closed = nullptr) const;
    CreaseId crease_id(Vec3 a, Vec3 b) const;
    bool resolve_segment(const CreaseId& id, Vec3& a, Vec3& b) const;
    std::vector<Vec3> resolve_chain(const CreaseId& id, int max_seg = 48, bool seed_only = false,
                                   bool* closed = nullptr) const;
    std::uint64_t tri_face_key(int tri) const;
    std::vector<std::vector<Vec3>> face_loops(Vec3 p, Vec3 n, float ndot = 0.86f, float plane_tol = 1.6f,
                                              int fid = 0) const;
    Hit raycast(const Ray& ray) const;
    bool empty() const { return indices.empty(); }
};

// Signed-distance primitives in local space, then transformed.
float sd_box(Vec3 p, Vec3 half);
float sd_sphere(Vec3 p, float r);
float sd_cylinder(Vec3 p, float r, float h); // along +Z, from 0..h
float sd_cone(Vec3 p, float r, float h);
float sd_polygon_xz(Vec2 p, const std::vector<Vec2>& poly); // 2D signed dist
float sd_extrude(Vec3 p, const std::vector<Vec2>& poly, float h, float taper_deg = 0, bool midplane = false);
float sd_revolve(Vec3 p, const std::vector<Vec2>& poly);
float sd_round_box(Vec3 p, Vec3 half, float r);
float sd_slot(Vec3 p, float length, float width, float height);
float sd_sweep(Vec3 p, const std::vector<Vec3>& path, float radius, const std::vector<Vec2>& profile);
float sd_coil(Vec3 p, float major, float minor, float pitch, float turns);
float sd_loft(Vec3 p, const std::vector<Vec2>& a, const std::vector<Vec2>& b, float h);
float sd_gear(Vec3 p, float r_tip, float r_bore, int teeth, float h);
float sd_thread(Vec3 p, float major, float pitch, float length, float depth, bool internal);
float sd_rib(Vec3 p, float length, float height, float thickness);
float sd_torus(Vec3 p, float major, float minor, float sweep_deg = 360.f);
float sd_pipe(Vec3 p, const std::vector<Vec3>& path, float outer, float inner);
float sd_hex(Vec3 p, float apothem, float h); // regular hex, across-flats = 2*apothem, 0..h
float sd_text(Vec3 p, const std::vector<Vec2>& strokes, float depth, float stroke);
float sd_capsule(Vec3 p, float r, float h); // Z from 0..h, hemispherical ends
float sd_wedge(Vec3 p, Vec3 size, int flip = 0); // tall at x=0 (flip=0) or x=W (flip=1)
float smin(float a, float b, float k);
float smax(float a, float b, float k);

enum class SdfKind : std::uint8_t {
    Box,
    Sphere,
    Cylinder,
    Cone,
    Extrude,
    Revolve,
    Sweep,
    Coil,
    Slot,
    Loft,
    Shell,
    Gear,
    Thread,
    Rib,
    Torus,
    Pipe,
    Thicken,
    Draft,
    Hex,
    PlaneCut,
    Text,
    Capsule,
    Wedge,
    EdgeBlend
};

struct SdfPrim {
    SdfKind kind = SdfKind::Box;
    Vec3 origin;
    Vec3 euler_deg; // Rx, Ry, Rz
    Vec3 size;      // box w/d/h, cyl r/h, sphere r, cone r/h
    std::vector<Vec2> profile; // local XY of sketch plane (we store as X/Y of plane)
    std::vector<Vec2> profile_b; // loft end
    std::vector<Vec3> path;    // sweep centerline (local)
    float height = 0;
    float round = 0; // rounded-box radius
    float taper = 0; // extrude draft, degrees
    int plane = 0; // 0=XY 1=XZ 2=YZ; shell open face 0..5
    int style = 0; // extrude: 1=midplane; thread: 1=internal
    std::uint32_t feature_id = 0;
    Mat4 xf = Mat4::identity();
    Mat4 inv = Mat4::identity();
    Aabb aabb;
    bool xform_ready = false;
    void prepare();
};

enum class BoolOp : std::uint8_t { NewBody, Join, Cut, Intersect };

struct SdfNode {
    BoolOp op = BoolOp::NewBody;
    SdfPrim prim;
    float blend = 0;
    bool chamfer = false;
};

struct SdfScene {
    std::vector<SdfNode> nodes; // in history order; NewBody starts a body
};

float eval_prim(const SdfPrim& prim, Vec3 p);
float eval_scene(const SdfScene& scene, Vec3 p, int upto = -1);
float apply_edge_blend(float d, Vec3 p, Vec3 a, Vec3 b, Vec3 n1, Vec3 n2, float r, bool chamfer);
Aabb bounds_prim(const SdfPrim& prim);
Aabb bounds_scene(const SdfScene& scene, int upto = -1);
void prepare_scene(SdfScene& scene);

Mesh tessellate_exact(const SdfPrim& prim, int quality = 160);
Mesh tessellate_sdf(const SdfScene& scene, int upto, int resolution, std::uint32_t paint_id);
Mesh rebuild_solid(const SdfScene& scene, int upto, int quality);
Mesh transformed(const Mesh& m, const Mat4& x);

void append_mesh(Mesh& dst, const Mesh& src);

} // namespace ax
