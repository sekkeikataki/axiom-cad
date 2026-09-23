#pragma once

#include "math.hpp"
#include <string>
#include <vector>

namespace ax {

enum class ConKind : std::uint8_t {
    Horz,
    Vert,
    DistW,
    DistH,
    Radius,
    Length,
    Coincident,
    Parallel,
    Perp,
    Equal,
    Tangent,
    Concentric
};

struct SkCon {
    ConKind kind = ConKind::Horz;
    int a = 0;      // entity index
    int b = -1;     // second entity
    int ea = 0;     // endpoint on a (0=a, 1=b)
    int eb = 0;
    float val = 0;
};

const char* con_name(ConKind k);
void sketch_auto_constrain(std::vector<struct SketchEnt>& ents, std::vector<SkCon>& cons, int last);
int sketch_solve(std::vector<struct SketchEnt>& ents, std::vector<SkCon>& cons, int iters = 36);

} // namespace ax
