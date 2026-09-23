#include "engineer.hpp"
#include "document.hpp"
#include "inspect.hpp"
#include "slicer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <ostream>
#include <thread>
#include <unordered_map>

namespace ax {

static void gword(std::ostream& o, char w, float v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%c%.3f", w, (double)v);
    o << b;
}

static void g3(std::ostream& o, float v) {
    char b[24];
    std::snprintf(b, sizeof(b), "%.3f", (double)v);
    o << b;
}

const char* analysis_mode_name(AnalysisMode m) {
    switch (m) {
    case AnalysisMode::Static: return "Static";
    case AnalysisMode::Modal: return "Modal";
    case AnalysisMode::Thermal: return "Thermal";
    }
    return "Static";
}

int estimate_dof(int n_free, int n_insert, int n_flush, int n_angle) {
    int d = n_free * 6 - n_insert * 3 - n_flush * 1 - n_angle * 1;
    return std::max(0, d);
}

FaceRef classify_face(const Mesh& m, const Hit& h) {
    FaceRef f;
    if (!h.hit) return f;
    f.ok = true;
    f.p = h.p;
    f.n = h.n.length2() > 1e-8f ? h.n.normalized() : Vec3{0, 0, 1};
    f.feature_id = (int)h.feature_id;
    float ax = std::fabs(f.n.x), ay = std::fabs(f.n.y), az = std::fabs(f.n.z);
    f.axis = az >= ax && az >= ay ? 2 : (ay >= ax ? 1 : 0);
    // refine normal from nearby same-feature triangles
    Vec3 acc = f.n;
    int n = 1;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        const Vertex& a = m.vertices[m.indices[i]];
        if (h.feature_id && a.feature_id != h.feature_id) continue;
        Vec3 c = (a.p + m.vertices[m.indices[i + 1]].p + m.vertices[m.indices[i + 2]].p) * (1.f / 3.f);
        if ((c - h.p).length2() > 36.f) continue;
        Vec3 e1 = m.vertices[m.indices[i + 1]].p - a.p;
        Vec3 e2 = m.vertices[m.indices[i + 2]].p - a.p;
        Vec3 nn = e1.cross(e2);
        if (nn.length2() < 1e-10f) continue;
        nn = nn.normalized();
        if (nn.dot(f.n) < 0.82f) continue;
        acc += nn;
        ++n;
    }
    if (n) f.n = (acc * (1.f / (float)n)).normalized();
    f.key = face_key(f.feature_id, f.n);
    return f;
}

FaceRef pick_largest_face(const Mesh& m, Vec3 prefer_n) {
    FaceRef best;
    if (m.indices.size() < 3) return best;
    prefer_n = prefer_n.length2() > 1e-8f ? prefer_n.normalized() : Vec3{0, 0, 1};
    float best_a = 0;
    Vec3 seed_p{}, seed_n = prefer_n;
    int seed_fid = 0;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 a = m.vertices[m.indices[i]].p;
        Vec3 b = m.vertices[m.indices[i + 1]].p;
        Vec3 c = m.vertices[m.indices[i + 2]].p;
        Vec3 n = (b - a).cross(c - a);
        float a2 = n.length2();
        if (a2 < 1e-16f) continue;
        float area = 0.5f * std::sqrt(a2);
        n = n * (1.f / std::sqrt(a2));
        if (n.dot(prefer_n) < 0.88f) continue;
        if (area > best_a) {
            best_a = area;
            seed_p = (a + b + c) * (1.f / 3.f);
            seed_n = n;
            seed_fid = (int)m.vertices[m.indices[i]].feature_id;
            best.ok = true;
        }
    }
    if (!best.ok) return best;
    Vec3 acc_p{}, acc_n{};
    float acc_a = 0;
    int nacc = 0;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 a = m.vertices[m.indices[i]].p;
        Vec3 b = m.vertices[m.indices[i + 1]].p;
        Vec3 c = m.vertices[m.indices[i + 2]].p;
        Vec3 n = (b - a).cross(c - a);
        if (n.length2() < 1e-16f) continue;
        float area = 0.5f * n.length();
        n = n.normalized();
        Vec3 mid = (a + b + c) * (1.f / 3.f);
        if (n.dot(seed_n) < 0.9f) continue;
        if (std::fabs((mid - seed_p).dot(seed_n)) > 1.4f) continue;
        acc_p += mid * area;
        acc_n += n * area;
        acc_a += area;
        ++nacc;
    }
    best.p = acc_a > 0 ? acc_p * (1.f / acc_a) : seed_p;
    best.n = acc_n.length2() > 0 ? acc_n.normalized() : seed_n;
    best.feature_id = seed_fid;
    float ax = std::fabs(best.n.x), ay = std::fabs(best.n.y), az = std::fabs(best.n.z);
    best.axis = az >= ax && az >= ay ? 2 : (ay >= ax ? 1 : 0);
    best.key = face_key(best.feature_id, best.n);
    (void)nacc;
    return best;
}

FaceRef resolve_face(const Mesh& m, std::uint64_t key, Vec3 hint_p, Vec3 hint_n) {
    FaceRef best;
    if (m.indices.size() < 3 || !key) return best;
    hint_n = hint_n.length2() > 1e-8f ? snap_face_normal(hint_n) : Vec3{0, 0, 1};
    float best_a = 0;
    Vec3 seed_p = hint_p, seed_n = hint_n;
    int seed_fid = 0;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 a = m.vertices[m.indices[i]].p;
        Vec3 b = m.vertices[m.indices[i + 1]].p;
        Vec3 c = m.vertices[m.indices[i + 2]].p;
        Vec3 n = (b - a).cross(c - a);
        if (n.length2() < 1e-16f) continue;
        n = n.normalized();
        int fid = (int)m.vertices[m.indices[i]].feature_id;
        if (face_key(fid, n) != key) continue;
        float area = 0.5f * (b - a).cross(c - a).length();
        Vec3 mid = (a + b + c) * (1.f / 3.f);
        float score = area - 0.02f * (mid - hint_p).length();
        if (score > best_a) {
            best_a = score;
            seed_p = mid;
            seed_n = n;
            seed_fid = fid;
            best.ok = true;
        }
    }
    if (!best.ok) return pick_largest_face(m, hint_n);
    Vec3 acc_p{}, acc_n{};
    float acc_a = 0;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 a = m.vertices[m.indices[i]].p;
        Vec3 b = m.vertices[m.indices[i + 1]].p;
        Vec3 c = m.vertices[m.indices[i + 2]].p;
        Vec3 n = (b - a).cross(c - a);
        if (n.length2() < 1e-16f) continue;
        n = n.normalized();
        int fid = (int)m.vertices[m.indices[i]].feature_id;
        if (face_key(fid, n) != key) continue;
        Vec3 mid = (a + b + c) * (1.f / 3.f);
        if (n.dot(seed_n) < 0.88f) continue;
        if (std::fabs((mid - seed_p).dot(seed_n)) > 2.2f) continue;
        float area = 0.5f * (b - a).cross(c - a).length();
        acc_p += mid * area;
        acc_n += n * area;
        acc_a += area;
    }
    best.p = acc_a > 0 ? acc_p * (1.f / acc_a) : seed_p;
    best.n = acc_n.length2() > 0 ? acc_n.normalized() : seed_n;
    best.feature_id = seed_fid;
    float ax = std::fabs(best.n.x), ay = std::fabs(best.n.y), az = std::fabs(best.n.z);
    best.axis = az >= ax && az >= ay ? 2 : (ay >= ax ? 1 : 0);
    best.key = key;
    best.ok = true;
    return best;
}

EdgeRef classify_edge(const Mesh& m, const Hit& h, float crease_deg) {
    EdgeRef e;
    if (!h.hit) return e;
    float best = 4.f * 4.f;
    if (m.edges.size() >= 2) {
        for (size_t i = 0; i + 1 < m.edges.size(); i += 2) {
            Vec3 a = m.edges[i], b = m.edges[i + 1];
            Vec3 ab = b - a;
            float L2 = ab.length2();
            if (L2 < 1e-8f) continue;
            float t = clamp((h.p - a).dot(ab) / L2, 0.f, 1.f);
            Vec3 q = a + ab * t;
            float d2 = (h.p - q).length2();
            if (d2 < best) {
                best = d2;
                e.a = a;
                e.b = b;
                e.dir = ab.normalized();
                e.ok = true;
            }
        }
    }
    if (!e.ok) {
        // fallback: longest edge of the hit triangle
        if (h.tri * 3 + 2 < m.indices.size()) {
            Vec3 p0 = m.vertices[m.indices[h.tri * 3]].p;
            Vec3 p1 = m.vertices[m.indices[h.tri * 3 + 1]].p;
            Vec3 p2 = m.vertices[m.indices[h.tri * 3 + 2]].p;
            float l01 = (p1 - p0).length2(), l12 = (p2 - p1).length2(), l20 = (p0 - p2).length2();
            if (l01 >= l12 && l01 >= l20) {
                e.a = p0;
                e.b = p1;
            } else if (l12 >= l20) {
                e.a = p1;
                e.b = p2;
            } else {
                e.a = p2;
                e.b = p0;
            }
            e.dir = (e.b - e.a).normalized();
            e.ok = true;
        }
    }
    e.feature_id = (int)h.feature_id;
    if (e.ok) {
        if (!m.topo_ok) const_cast<Mesh&>(m).build_topology();
        Vec3 ns[4];
        int nn = 0;
        auto near = [](Vec3 p, Vec3 q) { return (p - q).length2() < 0.36f; };
        for (size_t i = 0; i + 2 < m.indices.size() && nn < 4; i += 3) {
            Vec3 p0 = m.vertices[m.indices[i]].p;
            Vec3 p1 = m.vertices[m.indices[i + 1]].p;
            Vec3 p2 = m.vertices[m.indices[i + 2]].p;
            bool hit_e = (near(p0, e.a) && near(p1, e.b)) || (near(p0, e.b) && near(p1, e.a)) ||
                         (near(p1, e.a) && near(p2, e.b)) || (near(p1, e.b) && near(p2, e.a)) ||
                         (near(p2, e.a) && near(p0, e.b)) || (near(p2, e.b) && near(p0, e.a));
            if (!hit_e) continue;
            Vec3 n = (p1 - p0).cross(p2 - p0);
            if (n.length2() < 1e-10f) continue;
            n = n.normalized();
            bool dup = false;
            for (int k = 0; k < nn; ++k)
                if (ns[k].dot(n) > 0.92f) dup = true;
            if (dup) continue;
            ns[nn++] = n;
        }
        if (nn >= 1) e.n1 = ns[0];
        if (nn >= 2) e.n2 = ns[1];
        else if (nn == 1) {
            e.n2 = e.dir.cross(e.n1);
            if (e.n2.length2() < 1e-8f) e.n2 = {0, 1, 0};
            e.n2 = e.n2.normalized();
        }
        if (h.n.length2() > 1e-8f && e.n1.dot(h.n) < 0.2f && nn < 2) e.n1 = h.n.normalized();
        if (m.topo_ok) e.id = m.crease_id(e.a, e.b);
        if (e.id.ok) {
            e.n1 = e.id.n1;
            e.n2 = e.id.n2;
        }
    }
    (void)crease_deg;
    return e;
}

void face_outline(const Mesh& m, const FaceRef& f, std::vector<Vec3>& lines) {
    if (!f.ok) return;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 a = m.vertices[m.indices[i]].p;
        Vec3 b = m.vertices[m.indices[i + 1]].p;
        Vec3 c = m.vertices[m.indices[i + 2]].p;
        Vec3 n = (b - a).cross(c - a);
        if (n.length2() < 1e-10f) continue;
        n = n.normalized();
        if (n.dot(f.n) < 0.88f) continue;
        Vec3 mid = (a + b + c) * (1.f / 3.f);
        if (std::fabs((mid - f.p).dot(f.n)) > 1.2f) continue;
        if (f.feature_id && m.vertices[m.indices[i]].feature_id &&
            (int)m.vertices[m.indices[i]].feature_id != f.feature_id)
            continue;
        lines.push_back(a);
        lines.push_back(b);
        lines.push_back(b);
        lines.push_back(c);
        lines.push_back(c);
        lines.push_back(a);
    }
}

float face_separation(const FaceRef& a, const FaceRef& b) {
    if (!a.ok || !b.ok) return 0;
    return std::fabs((b.p - a.p).dot(a.n.normalized()));
}

void section_plane_lines(const Mesh& m, Vec3 n, float d, std::vector<Vec3>& lines) {
    n = n.length2() > 1e-10f ? n.normalized() : Vec3{0, 1, 0};
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 p[3] = {m.vertices[m.indices[i]].p, m.vertices[m.indices[i + 1]].p, m.vertices[m.indices[i + 2]].p};
        float s[3] = {n.dot(p[0]) - d, n.dot(p[1]) - d, n.dot(p[2]) - d};
        Vec3 hit[2];
        int nh = 0;
        for (int e = 0; e < 3 && nh < 2; ++e) {
            int a = e, b = (e + 1) % 3;
            if (s[a] * s[b] > 0.f) continue;
            float den = s[a] - s[b];
            if (std::fabs(den) < 1e-8f) continue;
            float t = s[a] / den;
            if (t < -0.02f || t > 1.02f) continue;
            hit[nh++] = p[a] + (p[b] - p[a]) * t;
        }
        if (nh == 2 && (hit[1] - hit[0]).length2() > 1e-6f) {
            lines.push_back(hit[0]);
            lines.push_back(hit[1]);
        }
    }
}

static bool tri_aabb_overlap(Vec3 a, Vec3 b, Vec3 c, const Aabb& box) {
    Aabb t;
    t.expand(a);
    t.expand(b);
    t.expand(c);
    return t.overlaps(box, 0.02f);
}

int mesh_tri_hits(const Mesh& A, const Mat4& xa, const Mesh& B, const Mat4& xb, int cap) {
    if (A.indices.size() < 3 || B.indices.size() < 3) return 0;
    Mesh wa = A, wb = B;
    auto xform = [](Mesh& m, const Mat4& x) {
        for (auto& v : m.vertices) v.p = x.transform_point(v.p);
        m.compute_bounds();
    };
    xform(wa, xa);
    xform(wb, xb);
    if (!wa.bounds.valid() || !wb.bounds.valid() || !wa.bounds.overlaps(wb.bounds, 0.05f)) return 0;
    int hits = 0;
    for (size_t i = 0; i + 2 < wa.indices.size() && hits < cap; i += 3) {
        Vec3 a = wa.vertices[wa.indices[i]].p;
        Vec3 b = wa.vertices[wa.indices[i + 1]].p;
        Vec3 c = wa.vertices[wa.indices[i + 2]].p;
        if (!tri_aabb_overlap(a, b, c, wb.bounds)) continue;
        Aabb ta;
        ta.expand(a);
        ta.expand(b);
        ta.expand(c);
        for (size_t j = 0; j + 2 < wb.indices.size(); j += 3) {
            Vec3 d = wb.vertices[wb.indices[j]].p;
            Vec3 e = wb.vertices[wb.indices[j + 1]].p;
            Vec3 f = wb.vertices[wb.indices[j + 2]].p;
            Aabb tb;
            tb.expand(d);
            tb.expand(e);
            tb.expand(f);
            if (!ta.overlaps(tb, 0.04f)) continue;
            Vec3 n = (b - a).cross(c - a);
            if (n.length2() < 1e-12f) continue;
            float da = n.dot(d - a), db = n.dot(e - a), dc = n.dot(f - a);
            if ((da > 0.06f && db > 0.06f && dc > 0.06f) || (da < -0.06f && db < -0.06f && dc < -0.06f)) continue;
            ++hits;
            break;
        }
    }
    return hits;
}

void repair_mesh(Mesh& m) {
    if (m.indices.size() < 3) return;
    std::vector<int> map(m.vertices.size(), -1);
    std::vector<Vertex> kept;
    kept.reserve(m.vertices.size());
    auto key = [](Vec3 p) {
        auto q = [](float v) { return (std::int32_t)std::lround(v * 50.f); };
        return ((std::uint64_t)(std::uint32_t)q(p.x) << 42) ^ ((std::uint64_t)(std::uint32_t)q(p.y) << 21) ^
               (std::uint32_t)q(p.z);
    };
    std::unordered_map<std::uint64_t, int> weld;
    for (size_t i = 0; i < m.vertices.size(); ++i) {
        std::uint64_t k = key(m.vertices[i].p);
        auto it = weld.find(k);
        if (it != weld.end())
            map[i] = it->second;
        else {
            map[i] = (int)kept.size();
            weld[k] = map[i];
            kept.push_back(m.vertices[i]);
        }
    }
    std::vector<std::uint32_t> idx;
    idx.reserve(m.indices.size());
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        int a = map[m.indices[i]], b = map[m.indices[i + 1]], c = map[m.indices[i + 2]];
        if (a < 0 || b < 0 || c < 0 || a == b || b == c || a == c) continue;
        Vec3 pa = kept[a].p, pb = kept[b].p, pc = kept[c].p;
        if ((pb - pa).cross(pc - pa).length2() < 1e-12f) continue;
        idx.push_back((std::uint32_t)a);
        idx.push_back((std::uint32_t)b);
        idx.push_back((std::uint32_t)c);
    }
    m.vertices = std::move(kept);
    m.indices = std::move(idx);
    m.compute_bounds();
    m.compute_smooth_normals();
    m.extract_crease_edges();
}
