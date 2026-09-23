#pragma once

#include "engineer.hpp"
#include <string>
#include <vector>

namespace ax {

struct FitLimits {
    bool ok = false;
    char name[8]{};
    bool hole = true;
    int it = 7;
    float nom = 0;
    float es = 0, ei = 0; // mm (hole ES/EI or shaft es/ei)
    float max_d = 0, min_d = 0;
    float mid_d = 0;
};

struct PairFit {
    bool ok = false;
    FitLimits hole, shaft;
    float cmin = 0, cmax = 0; // + clearance, − interference
    enum class Kind { Clearance, Transition, Interference } kind = Kind::Clearance;
};

struct StackSeg {
    std::string name;
    float nom = 0;
    float plus = 0.1f;
    float minus = 0.1f;
    int sign = 1;
};

struct StackResult {
    float nom = 0;
    float wc_min = 0, wc_max = 0;
    float rss = 0;
    std::vector<StackSeg> segs;
};

struct InspRow {
    std::string item;
    std::string type;
    float nom = 0, plus = 0, minus = 0, actual = 0;
    bool pass = true;
    std::string note;
};

struct DfmIssue {
    const char* sev = "INFO";
    std::string msg;
};

struct Iso286Name {
    const char* name;
    bool hole;
};

const Iso286Name* iso286_names(int* n);
const char* default_hole_fit(float dia_mm);
FitLimits iso286_limits(float nom_mm, const char* fit);
PairFit iso286_pair(float nom_mm, const char* hole_fit, const char* shaft_fit);
const char* pair_kind_name(PairFit::Kind k);

StackResult stack_up(const std::vector<StackSeg>& segs);

const char* gdt_char_name(GdtChar c);
const char* gdt_char_tag(GdtChar c);
GdtChar parse_gdt_char(const char* s);

float measure_hole_dia(const SdfScene& scene, Vec3 origin, Vec3 axis, float guess_dia);
float measure_flatness(const Mesh& m, const FaceRef& f);
float measure_perp_deg(Vec3 a, Vec3 b);

bool write_turn_gcode(const std::string& path, const Mesh& mesh, const SdfScene* scene, const TurnSettings& s,
                      std::string* err);
bool write_inspection_csv(const std::string& path, const std::vector<InspRow>& rows, std::string* err);
bool write_dfm_report(const std::string& path, const std::vector<DfmIssue>& issues, std::string* err);

} // namespace ax
