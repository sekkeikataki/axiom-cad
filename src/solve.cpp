#include "solve.hpp"
#include "document.hpp"
#include <cmath>

namespace ax {

const char* con_name(ConKind k) {
    switch (k) {
    case ConKind::Horz: return "Horizontal";
    case ConKind::Vert: return "Vertical";
    case ConKind::DistW: return "Width";
    case ConKind::DistH: return "Height";
    case ConKind::Radius: return "Radius";
    case ConKind::Length: return "Length";
    case ConKind::Coincident: return "Coincident";
    case ConKind::Parallel: return "Parallel";
    case ConKind::Perp: return "Perpendicular";
    case ConKind::Equal: return "Equal";
    case ConKind::Tangent: return "Tangent";
    case ConKind::Concentric: return "Concentric";
    }
    return "Constraint";
}

static Vec2* endp(SketchEnt& e, int which) { return which ? &e.b : &e.a; }

void sketch_auto_constrain(std::vector<SketchEnt>& ents, std::vector<SkCon>& cons, int last) {
    if (last < 0 || last >= (int)ents.size()) return;
    const SketchEnt& e = ents[last];
    if (e.type == SketchEnt::Type::Rect) {
        cons.push_back({ConKind::DistW, last, -1, 0, 0, std::fabs(e.b.x - e.a.x)});
        cons.push_back({ConKind::DistH, last, -1, 0, 0, std::fabs(e.b.y - e.a.y)});
    } else if (e.type == SketchEnt::Type::Circle) {
        cons.push_back({ConKind::Radius, last, -1, 0, 0, e.r > 0 ? e.r : (e.b - e.a).length()});
    } else if (e.type == SketchEnt::Type::Arc) {
        cons.push_back({ConKind::Radius, last, -1, 0, 0, (e.b - e.a).length()});
    } else if (e.type == SketchEnt::Type::Line) {
        Vec2 d = e.b - e.a;
        if (std::fabs(d.y) < 0.35f) cons.push_back({ConKind::Horz, last, -1, 0, 0, 0});
        else if (std::fabs(d.x) < 0.35f) cons.push_back({ConKind::Vert, last, -1, 0, 0, 0});
        cons.push_back({ConKind::Length, last, -1, 0, 0, d.length()});
        for (int i = 0; i < (int)ents.size(); ++i) {
            if (i == last) continue;
            if (ents[i].type != SketchEnt::Type::Circle) continue;
            Vec2 ab = e.b - e.a;
            float L = ab.length();
            if (L < 1e-5f) continue;
            Vec2 n = {-ab.y / L, ab.x / L};
            float dist = std::fabs((ents[i].a - e.a).dot(n));
            float r = ents[i].r > 0.05f ? ents[i].r : (ents[i].b - ents[i].a).length();
            if (std::fabs(dist - r) < 0.6f) cons.push_back({ConKind::Tangent, last, i, 0, 0, 0});
        }
    }
    if (e.type == SketchEnt::Type::Circle) {
        for (int i = 0; i < (int)ents.size(); ++i) {
            if (i == last || ents[i].type != SketchEnt::Type::Circle) continue;
            if ((ents[i].a - e.a).length() < 0.6f) cons.push_back({ConKind::Concentric, last, i, 0, 0, 0});
        }
    }
}

int sketch_solve(std::vector<SketchEnt>& ents, std::vector<SkCon>& cons, int iters) {
    if (ents.empty() || cons.empty()) return 0;
    int applied = 0;
    for (int it = 0; it < iters; ++it) {
        for (auto& c : cons) {
            if (c.a < 0 || c.a >= (int)ents.size()) continue;
            SketchEnt& A = ents[c.a];
            switch (c.kind) {
            case ConKind::Horz:
                if (A.type == SketchEnt::Type::Line) {
                    float y = 0.5f * (A.a.y + A.b.y);
                    A.a.y = A.b.y = y;
                    ++applied;
                }
                break;
            case ConKind::Vert:
                if (A.type == SketchEnt::Type::Line) {
                    float x = 0.5f * (A.a.x + A.b.x);
                    A.a.x = A.b.x = x;
                    ++applied;
                }
                break;
            case ConKind::DistW:
                if (A.type == SketchEnt::Type::Rect) {
                    float s = (A.b.x >= A.a.x) ? 1.f : -1.f;
                    A.b.x = A.a.x + s * std::max(0.1f, c.val);
                    ++applied;
                }
                break;
            case ConKind::DistH:
                if (A.type == SketchEnt::Type::Rect) {
                    float s = (A.b.y >= A.a.y) ? 1.f : -1.f;
                    A.b.y = A.a.y + s * std::max(0.1f, c.val);
                    ++applied;
                }
                break;
            case ConKind::Radius:
                if (A.type == SketchEnt::Type::Circle) {
                    A.r = std::max(0.1f, c.val);
                    ++applied;
                } else if (A.type == SketchEnt::Type::Arc) {
                    Vec2 d = A.b - A.a;
                    float L = d.length();
                    if (L < 1e-6f) {
                        d = {1, 0};
                        L = 1;
                    }
                    A.b = A.a + d * (std::max(0.1f, c.val) / L);
                    ++applied;
                }
                break;
            case ConKind::Length:
                if (A.type == SketchEnt::Type::Line) {
                    Vec2 d = A.b - A.a;
                    float L = d.length();
                    if (L < 1e-6f) d = {1, 0}, L = 1;
                    A.b = A.a + d * (std::max(0.1f, c.val) / L);
                    ++applied;
                }
                break;
            case ConKind::Coincident:
                if (c.b < 0 || c.b >= (int)ents.size()) break;
                {
                    Vec2* pa = endp(A, c.ea);
                    Vec2* pb = endp(ents[c.b], c.eb);
                    Vec2 m = (*pa + *pb) * 0.5f;
                    *pa = *pb = m;
                    ++applied;
                }
                break;
            case ConKind::Parallel:
            case ConKind::Perp:
                if (c.b < 0 || c.b >= (int)ents.size()) break;
                if (A.type != SketchEnt::Type::Line || ents[c.b].type != SketchEnt::Type::Line) break;
                {
                    SketchEnt& B = ents[c.b];
                    Vec2 da = (A.b - A.a).normalized();
                    float len = (B.b - B.a).length();
                    Vec2 db = c.kind == ConKind::Perp ? Vec2{-da.y, da.x} : da;
                    B.b = B.a + db * std::max(0.1f, len);
                    ++applied;
                }
                break;
            case ConKind::Equal:
                if (c.b < 0 || c.b >= (int)ents.size()) break;
                if (A.type == SketchEnt::Type::Circle && ents[c.b].type == SketchEnt::Type::Circle) {
                    float r = 0.5f * (A.r + ents[c.b].r);
                    A.r = ents[c.b].r = std::max(0.1f, r);
                    ++applied;
                } else if (A.type == SketchEnt::Type::Line && ents[c.b].type == SketchEnt::Type::Line) {
                    float la = (A.b - A.a).length(), lb = (ents[c.b].b - ents[c.b].a).length();
                    float L = 0.5f * (la + lb);
                    if (la > 1e-6f) A.b = A.a + (A.b - A.a) * (L / la);
                    if (lb > 1e-6f) ents[c.b].b = ents[c.b].a + (ents[c.b].b - ents[c.b].a) * (L / lb);
                    ++applied;
                }
                break;
            case ConKind::Tangent:
                if (c.b < 0 || c.b >= (int)ents.size()) break;
                {
                    SketchEnt* circ = nullptr;
                    SketchEnt* line = nullptr;
                    if (A.type == SketchEnt::Type::Circle && ents[c.b].type == SketchEnt::Type::Line) {
                        circ = &A;
                        line = &ents[c.b];
                    } else if (A.type == SketchEnt::Type::Line && ents[c.b].type == SketchEnt::Type::Circle) {
                        line = &A;
                        circ = &ents[c.b];
                    }
                    if (circ && line) {
                        Vec2 ab = line->b - line->a;
                        float L = ab.length();
                        if (L > 1e-5f) {
                            Vec2 n{-ab.y / L, ab.x / L};
                            float r = circ->r > 0.05f ? circ->r : (circ->b - circ->a).length();
                            float dist = (circ->a - line->a).dot(n);
                            float target = (dist >= 0 ? r : -r);
                            float delta = dist - target;
                            line->a += n * (-delta * 0.45f);
                            line->b += n * (-delta * 0.45f);
                            circ->a += n * (delta * 0.45f);
                            ++applied;
                        }
                    }
                }
                break;
            case ConKind::Concentric:
                if (c.b < 0 || c.b >= (int)ents.size()) break;
                if (A.type == SketchEnt::Type::Circle && ents[c.b].type == SketchEnt::Type::Circle) {
                    Vec2 m = (A.a + ents[c.b].a) * 0.5f;
                    A.a = ents[c.b].a = m;
                    ++applied;
                }
                break;
            }
        }
    }
    return applied;
}

} // namespace ax
