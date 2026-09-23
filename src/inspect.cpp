#include "inspect.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace ax {

static int size_band(float d) {
    if (d <= 3.f) return 0;
    if (d <= 6.f) return 1;
    if (d <= 10.f) return 2;
    if (d <= 18.f) return 3;
    if (d <= 30.f) return 4;
    if (d <= 50.f) return 5;
    if (d <= 80.f) return 6;
    if (d <= 120.f) return 7;
    if (d <= 180.f) return 8;
    return 9;
}

static int it_um(float d, int it) {
    // ISO 286-1 Table 5, IT5..IT12, bands 0–3 … 180–250
    static const int tab[10][8] = {
        {4, 6, 10, 14, 25, 40, 60, 100},      {5, 8, 12, 18, 30, 48, 75, 120},
        {6, 9, 15, 22, 36, 58, 90, 150},      {8, 11, 18, 27, 43, 70, 110, 180},
        {9, 13, 21, 33, 52, 84, 130, 210},    {11, 16, 25, 39, 62, 100, 160, 250},
        {13, 19, 30, 46, 74, 120, 190, 300},  {15, 22, 35, 54, 87, 140, 220, 350},
        {18, 25, 40, 63, 100, 160, 250, 400}, {20, 29, 46, 72, 115, 185, 290, 460},
    };
    int col = std::clamp(it, 5, 12) - 5;
    return tab[size_band(d)][col];
}

static int fund_um(float d, char L) {
    int b = size_band(d);
    L = (char)std::tolower((unsigned char)L);
    static const int g[] = {-2, -4, -5, -6, -7, -9, -10, -12, -14, -15};
    static const int f[] = {-6, -10, -13, -16, -20, -25, -30, -36, -43, -50};
    static const int k[] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 4};
    static const int n[] = {4, 8, 10, 12, 15, 17, 20, 23, 27, 31};
    static const int p[] = {6, 12, 15, 18, 22, 26, 32, 37, 43, 50};
    static const int s[] = {14, 19, 23, 28, 35, 43, 53, 66, 79, 92};
    if (L == 'h') return 0;
    if (L == 'g') return g[b];
    if (L == 'f') return f[b];
    if (L == 'k') return k[b];
    if (L == 'n') return n[b];
    if (L == 'p') return p[b];
    if (L == 's') return s[b];
    return 0;
}

static bool parse_fit(const char* s, char& letter, int& it, bool& hole) {
    if (!s || !*s) return false;
    while (*s == ' ' || *s == '\t') ++s;
    if (!*s) return false;
    letter = (char)std::toupper((unsigned char)*s++);
    if (letter == 'J' && (*s == 's' || *s == 'S')) ++s;
    it = 0;
    while (*s >= '0' && *s <= '9') it = it * 10 + (*s++ - '0');
    if (it < 5 || it > 12) it = 7;
    hole = (letter >= 'A' && letter <= 'Z' && letter != 'J');
    // ISO: A–H holes (uppercase), a–h shafts. We treat stored uppercase H as hole,
    // lowercase input as shaft. parse already uppercased — callers pass "H7" / "g6".
    return letter >= 'A' && letter <= 'Z';
}

const Iso286Name* iso286_names(int* n) {
    static const Iso286Name k[] = {
        {"H7", true},  {"H8", true},  {"H11", true}, {"G7", true}, {"Js7", true},
        {"h6", false}, {"g6", false}, {"f7", false}, {"k6", false}, {"n6", false},
        {"p6", false}, {"s7", false},
    };
    if (n) *n = (int)(sizeof(k) / sizeof(k[0]));
    return k;
}

const char* default_hole_fit(float dia_mm) {
    if (dia_mm >= 12.f) return "H7";
    if (dia_mm >= 6.f) return "H8";
    return "H11";
}

FitLimits iso286_limits(float nom_mm, const char* fit) {
    FitLimits L;
    if (nom_mm < 0.2f || !fit || !fit[0]) return L;
    // allow H7/g6 — take first token
    char buf[16]{};
    int n = 0;
    for (const char* p = fit; *p && *p != '/' && n < 15; ++p) buf[n++] = *p;
    bool hole = true;
    char letter = 'H';
    int it = 7;
    // detect shaft if original started lowercase
    bool lower = fit[0] >= 'a' && fit[0] <= 'z';
    if (!parse_fit(buf, letter, it, hole)) return L;
    if (lower) hole = false;
    if (letter == 'H' || letter == 'G' || letter == 'J') hole = !lower;
    if (letter >= 'A' && letter <= 'H' && !lower) hole = true;
    if (lower) hole = false;

    int IT = it_um(nom_mm, it);
    float itmm = IT * 0.001f;
    char lc = (char)std::tolower((unsigned char)letter);
    std::snprintf(L.name, sizeof(L.name), "%c%d", hole ? (char)std::toupper((unsigned char)letter) : lc, it);
    L.ok = true;
    L.hole = hole;
    L.it = it;
    L.nom = nom_mm;

    if (letter == 'J') {
        L.ei = -0.5f * itmm;
        L.es = 0.5f * itmm;
    } else if (hole) {
        if (lc == 'h') {
            L.ei = 0;
            L.es = itmm;
        } else if (lc == 'g') {
            L.ei = -fund_um(nom_mm, 'g') * 0.001f; // g is negative; hole G is +|g|
            L.es = L.ei + itmm;
        } else {
            L.ei = 0;
            L.es = itmm;
        }
    } else {
        if (lc == 'h' || lc == 'g' || lc == 'f') {
            L.es = fund_um(nom_mm, lc) * 0.001f;
            L.ei = L.es - itmm;
        } else {
            L.ei = fund_um(nom_mm, lc) * 0.001f;
            L.es = L.ei + itmm;
        }
    }
    L.max_d = nom_mm + L.es;
    L.min_d = nom_mm + L.ei;
    if (L.max_d < L.min_d) std::swap(L.max_d, L.min_d);
    L.mid_d = 0.5f * (L.max_d + L.min_d);
    return L;
}

PairFit iso286_pair(float nom_mm, const char* hole_fit, const char* shaft_fit) {
    PairFit p;
    p.hole = iso286_limits(nom_mm, hole_fit);
    p.shaft = iso286_limits(nom_mm, shaft_fit);
    p.ok = p.hole.ok && p.shaft.ok;
    if (!p.ok) return p;
    p.cmin = p.hole.min_d - p.shaft.max_d;
    p.cmax = p.hole.max_d - p.shaft.min_d;
    if (p.cmin >= 0.f)
        p.kind = PairFit::Kind::Clearance;
    else if (p.cmax <= 0.f)
        p.kind = PairFit::Kind::Interference;
    else
        p.kind = PairFit::Kind::Transition;
    return p;
}

const char* pair_kind_name(PairFit::Kind k) {
    if (k == PairFit::Kind::Interference) return "interference";
    if (k == PairFit::Kind::Transition) return "transition";
    return "clearance";
}

StackResult stack_up(const std::vector<StackSeg>& segs) {
    StackResult r;
    r.segs = segs;
    float var = 0;
    for (const auto& s : segs) {
        float n = s.nom * (float)s.sign;
        r.nom += n;
        r.wc_max += n + (s.sign > 0 ? s.plus : s.minus);
        r.wc_min += n - (s.sign > 0 ? s.minus : s.plus);
        float a = 0.5f * (s.plus + s.minus);
        var += a * a;
    }
    r.rss = std::sqrt(std::max(0.f, var));
    return r;
}

const char* gdt_char_name(GdtChar c) {
    switch (c) {
    case GdtChar::Position: return "Position";
    case GdtChar::Flatness: return "Flatness";
    case GdtChar::Perpendicularity: return "Perpendicularity";
    case GdtChar::Parallelism: return "Parallelism";
    case GdtChar::Cylindricity: return "Cylindricity";
    case GdtChar::Circularity: return "Circularity";
    case GdtChar::Concentricity: return "Concentricity";
    case GdtChar::Profile: return "Profile";
    }
    return "Position";
}

const char* gdt_char_tag(GdtChar c) {
    switch (c) {
    case GdtChar::Position: return "POS";
    case GdtChar::Flatness: return "FLT";
    case GdtChar::Perpendicularity: return "PERP";
    case GdtChar::Parallelism: return "PAR";
    case GdtChar::Cylindricity: return "CYL";
    case GdtChar::Circularity: return "CIR";
    case GdtChar::Concentricity: return "CON";
    case GdtChar::Profile: return "PRF";
    }
    return "POS";
}

GdtChar parse_gdt_char(const char* s) {
    if (!s) return GdtChar::Position;
    if (!std::strcmp(s, "FLT") || !std::strcmp(s, "Flatness")) return GdtChar::Flatness;
    if (!std::strcmp(s, "PERP") || !std::strcmp(s, "Perpendicularity")) return GdtChar::Perpendicularity;
    if (!std::strcmp(s, "PAR") || !std::strcmp(s, "Parallelism")) return GdtChar::Parallelism;
    if (!std::strcmp(s, "CYL") || !std::strcmp(s, "Cylindricity")) return GdtChar::Cylindricity;
    if (!std::strcmp(s, "CIR") || !std::strcmp(s, "Circularity")) return GdtChar::Circularity;
    if (!std::strcmp(s, "CON") || !std::strcmp(s, "Concentricity")) return GdtChar::Concentricity;
    if (!std::strcmp(s, "PRF") || !std::strcmp(s, "Profile")) return GdtChar::Profile;
    return GdtChar::Position;
}

float measure_hole_dia(const SdfScene& scene, Vec3 origin, Vec3 axis, float guess_dia) {
    if (scene.nodes.empty()) return 0;
    if (axis.length2() < 1e-10f) axis = {0, 0, 1};
    else axis = axis.normalized();
    Vec3 ref = std::fabs(axis.z) < 0.9f ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
    Vec3 u = axis.cross(ref).normalized();
    Vec3 v = axis.cross(u);
    Vec3 c = origin + axis * std::max(6.f, guess_dia * 0.6f);
    float best = 1e9f;
    float step = std::max(1.4f, guess_dia * 0.35f);
    for (int s = 0; s < 18; ++s) {
        Vec3 p = origin + axis * (1.2f + step * (float)s);
        float d = eval_scene(scene, p);
        if (d <= 0.02f) continue;
        float err = std::fabs(d - guess_dia * 0.45f);
        if (err < best) {
            best = err;
            c = p;
        }
    }
    float acc = 0;
    int n = 0;
    float hi0 = std::max(guess_dia * 1.6f, guess_dia + 4.f);
    for (int i = 0; i < 16; ++i) {
        float a = kTau * (float)i / 16.f;
        Vec3 dir = u * std::cos(a) + v * std::sin(a);
        float lo = 0.05f, hi = hi0;
        for (int k = 0; k < 20; ++k) {
            float mid = 0.5f * (lo + hi);
            float d = eval_scene(scene, c + dir * mid);
            if (d > 0.f) lo = mid;
            else hi = mid;
        }
        acc += lo + hi;
        ++n;
    }
    float dia = n ? acc / (float)n : 0.f;
    if (guess_dia > 0.5f && (dia < guess_dia * 0.45f || dia > guess_dia * 1.7f)) return 0.f;
    return dia;
}

float measure_flatness(const Mesh& m, const FaceRef& f) {
    if (!f.ok || m.indices.size() < 3) return 0;
    Vec3 n = f.n.length2() > 1e-10f ? f.n.normalized() : Vec3{0, 0, 1};
    float d0 = f.p.dot(n);
    float mn = 1e9f, mx = -1e9f;
    int hits = 0;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        Vec3 a = m.vertices[m.indices[i]].p;
        Vec3 b = m.vertices[m.indices[i + 1]].p;
        Vec3 c = m.vertices[m.indices[i + 2]].p;
        Vec3 nn = (b - a).cross(c - a);
        if (nn.length2() < 1e-16f) continue;
        nn = nn.normalized();
        if (nn.dot(n) < 0.97f) continue;
        Vec3 mid = (a + b + c) * (1.f / 3.f);
        if (std::fabs((mid - f.p).dot(n)) > 0.7f) continue;
        auto acc = [&](Vec3 p) {
            float d = p.dot(n) - d0;
            mn = std::min(mn, d);
            mx = std::max(mx, d);
            ++hits;
        };
        acc(a);
        acc(b);
        acc(c);
    }
    if (hits < 6) return 0;
    return mx - mn;
}

float measure_perp_deg(Vec3 a, Vec3 b) {
    if (a.length2() < 1e-12f || b.length2() < 1e-12f) return 90.f;
    float c = std::fabs(a.normalized().dot(b.normalized()));
    c = clamp(c, 0.f, 1.f);
    return std::acos(c) * (180.f / kPi);
}

static void gword(std::ostream& o, char w, float v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%c%.3f", w, (double)v);
    o << b;
}

bool write_turn_gcode(const std::string& path, const Mesh& mesh, const SdfScene* scene, const TurnSettings& in,
                      std::string* err) {
    if (mesh.empty() || !mesh.bounds.valid()) {
        if (err) *err = "Nothing to turn — the part has no solid.";
        return false;
    }
    std::ofstream out(path);
    if (!out) {
        if (err) *err = "Could not write " + path;
        return false;
    }
    TurnSettings s = in;
    Aabb b = mesh.bounds;
    Vec3 axis{0, 0, 1};
    Vec3 orig{(b.mn.x + b.mx.x) * 0.5f, (b.mn.y + b.mx.y) * 0.5f, 0};
    float rmax = 0;
    for (const auto& v : mesh.vertices) {
        Vec3 d = v.p - orig;
        d.z = 0;
        rmax = std::max(rmax, d.length());
    }
    float z0 = b.mn.z, z1 = b.mx.z;
    float stock_d = s.stock_d > 0.4f ? s.stock_d : rmax * 2.f + 3.f;
    float stock_len = s.stock_len > 0.4f ? s.stock_len : (z1 - z0) + 2.f;
    float finish = std::max(0.05f, s.finish);
    float doc = std::max(0.15f, s.doc);
    int feed = std::max(20, (int)std::lround(s.feed * 60.f * 10.f)); // 0.18 mm/rev → display as mm/min-ish
    if (s.feed < 2.f) feed = std::max(20, (int)std::lround(s.feed * 800.f));

    std::vector<Vec2> prof;
    int nz = 36;
    for (int i = 0; i <= nz; ++i) {
        float z = z1 - (z1 - z0) * ((float)i / (float)nz);
        float rr = 0;
        if (scene && !scene->nodes.empty()) {
            for (int k = 0; k < 24; ++k) {
                float a = kTau * (float)k / 24.f;
                Vec3 dir{std::cos(a), std::sin(a), 0};
                float lo = 0.05f, hi = rmax + 8.f;
                for (int it = 0; it < 16; ++it) {
                    float mid = 0.5f * (lo + hi);
                    float d = eval_scene(*scene, orig + dir * mid + Vec3{0, 0, z});
                    if (d < 0.f) lo = mid;
                    else hi = mid;
                }
                rr = std::max(rr, 0.5f * (lo + hi));
            }
        } else {
            for (const auto& v : mesh.vertices) {
                if (std::fabs(v.p.z - z) > (z1 - z0) / (float)nz + 0.6f) continue;
                Vec3 d = v.p - orig;
                d.z = 0;
                rr = std::max(rr, d.length());
            }
        }
        if (rr < 0.2f) rr = rmax;
        prof.push_back({rr, z});
    }

    out << "; Axiom 2-axis lathe  ·  Fanuc / LinuxCNC  ·  X=diameter  Z=axis\n";
    char sb[96];
    std::snprintf(sb, sizeof(sb), "; stock Ø%.3f  length %.3f  finish %.3f  DOC %.3f\n", (double)stock_d,
                  (double)stock_len, (double)finish, (double)doc);
    out << sb;
    out << "G90 G21 G18 G40 G99\n";
    out << "G97 S" << std::max(400, (int)(s.sfm * 1000.f / std::max(8.f, stock_d * 3.1416f))) << " M3\n";
    out << "G0 ";
    gword(out, 'X', stock_d + s.safe * 2.f);
    out << " ";
    gword(out, 'Z', z1 + s.safe);
    out << "\n;TYPE:Face\n";
    float zface = z1;
    float xstock = stock_d;
    while (zface > z1 - 0.02f && zface > z0) {
        out << "G0 ";
        gword(out, 'X', xstock + 1.f);
        out << " ";
        gword(out, 'Z', zface + 0.4f);
        out << "\nG1 ";
        gword(out, 'Z', zface);
        out << " F" << feed << "\nG1 ";
        gword(out, 'X', -0.2f);
        out << "\nG0 ";
        gword(out, 'X', xstock + 1.f);
        out << "\n";
        break;
    }
    out << ";TYPE:OD rough\n";
    float rpart = 0;
    for (auto& p : prof) rpart = std::max(rpart, p.x);
    float r = stock_d * 0.5f;
    float rfin = rpart + finish;
    while (r > rfin + 0.02f) {
        r = std::max(rfin, r - doc);
        out << "G0 ";
        gword(out, 'X', r * 2.f + 0.4f);
        out << " ";
        gword(out, 'Z', z1 + 0.4f);
        out << "\nG1 ";
        gword(out, 'X', r * 2.f);
        out << " F" << feed << "\nG1 ";
        gword(out, 'Z', z0);
        out << "\nG0 ";
        gword(out, 'X', r * 2.f + 1.2f);
        out << "\nG0 ";
        gword(out, 'Z', z1 + 0.4f);
        out << "\n";
    }
    out << ";TYPE:OD finish\n";
    if (!prof.empty()) {
        out << "G0 ";
        gword(out, 'X', prof.front().x * 2.f + 0.6f);
        out << " ";
        gword(out, 'Z', prof.front().y + 0.3f);
        out << "\n";
        for (const auto& p : prof) {
            out << "G1 ";
            gword(out, 'X', p.x * 2.f);
            out << " ";
            gword(out, 'Z', p.y);
            out << " F" << feed << "\n";
        }
    }
    out << "G0 ";
    gword(out, 'X', stock_d + s.safe * 2.f);
    out << " ";
    gword(out, 'Z', z1 + s.safe);
    out << "\nM5\nM30\n";
    (void)axis;
    return true;
}

bool write_inspection_csv(const std::string& path, const std::vector<InspRow>& rows, std::string* err) {
    std::ofstream out(path);
    if (!out) {
        if (err) *err = "Could not write " + path;
        return false;
    }
    out << "item,type,nominal,plus,minus,actual,pass,note\n";
    for (const auto& r : rows) {
        out << r.item << "," << r.type << "," << r.nom << "," << r.plus << "," << r.minus << "," << r.actual << ","
            << (r.pass ? "PASS" : "FAIL") << "," << r.note << "\n";
    }
    return true;
}

bool write_dfm_report(const std::string& path, const std::vector<DfmIssue>& issues, std::string* err) {
    std::ofstream out(path);
    if (!out) {
        if (err) *err = "Could not write " + path;
        return false;
    }
    out << "Axiom DFM report\n";
    int w = 0, f = 0;
    for (const auto& i : issues) {
        if (!std::strcmp(i.sev, "WARN")) ++w;
        if (!std::strcmp(i.sev, "FAIL")) ++f;
        out << i.sev << "  " << i.msg << "\n";
    }
    out << "summary  issues=" << issues.size() << "  warnings=" << w << "  fails=" << f << "\n";
    return true;
}

} // namespace ax
