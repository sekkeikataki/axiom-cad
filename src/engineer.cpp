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
