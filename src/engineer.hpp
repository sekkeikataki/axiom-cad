#pragma once

#include "mesh.hpp"
#include "material.hpp"
#include <string>
#include <vector>

namespace ax {

struct FaceRef {
    bool ok = false;
    Vec3 p;
    Vec3 n{0, 0, 1};
    int feature_id = 0;
    int axis = 2; // dominant world axis
    std::uint64_t key = 0;
};

struct EdgeRef {
    bool ok = false;
    Vec3 a, b;
    Vec3 dir{1, 0, 0};
    Vec3 n1{0, 0, 1};
    Vec3 n2{0, 1, 0};
    int feature_id = 0;
    CreaseId id;
};

FaceRef resolve_face(const Mesh& m, std::uint64_t key, Vec3 hint_p, Vec3 hint_n);

struct DrivenDim {
    int id = 0;
    int param_id = 0;
    Vec3 a, b;
    std::string name;
};

enum class AnalysisMode : std::uint8_t { Static, Modal, Thermal };

struct AnalysisSpec {
    AnalysisMode mode = AnalysisMode::Static;
    bool use_faces = false;
    Vec3 fix_p{}, fix_n{0, 0, -1};
    Vec3 load_p{}, load_n{0, 0, 1};
    Vec3 load_dir{0, 0, -1};
    float force = 25000;
    float t_hot = 80, t_cold = 20;
    int grid = 28;
    int slot = 0; // 0 primary, 1 second load case
};

struct MillSettings {
    float tool_d = 6.f;
    float stepover = 0.45f;
    float stepdown = 1.5f;
    float feed_mms = 20.f;
    float plunge_mms = 8.f;
    float safe_z = 5.f;
    float stock_z = 0; // 0 = mesh top
    int finish_passes = 1;
    bool use_face = false;
    FaceRef face;
};

enum class GdtChar : std::uint8_t {
    Position,
    Flatness,
    Perpendicularity,
    Parallelism,
    Cylindricity,
    Circularity,
    Concentricity,
    Profile
};

struct GdtFrame {
    int id = 0;
    GdtChar ch = GdtChar::Position;
    float tol = 0.1f;
    bool diameter = false;
    char datums[4] = {};
    int feature_id = 0;
    FaceRef face;
    std::string note;
};

struct DatumFeat {
    char letter = 'A';
    FaceRef face;
    int feature_id = 0;
};

struct TurnSettings {
    float tool_r = 0.4f;
    float doc = 0.8f;
    float feed = 0.18f;
    float sfm = 180.f;
    float stock_d = 0;
    float stock_len = 0;
    float finish = 0.2f;
    float safe = 2.f;
};

FaceRef classify_face(const Mesh& m, const Hit& h);
EdgeRef classify_edge(const Mesh& m, const Hit& h, float crease_deg = 38.f);
FaceRef pick_largest_face(const Mesh& m, Vec3 prefer_n = {0, 0, 1});
void face_outline(const Mesh& m, const FaceRef& f, std::vector<Vec3>& lines);
float face_separation(const FaceRef& a, const FaceRef& b);

int mesh_tri_hits(const Mesh& A, const Mat4& xa, const Mesh& B, const Mat4& xb, int cap = 48);
void repair_mesh(Mesh& m);

StressResult analyze_case(const SdfScene& scene, Mesh& paint, const Material& mat, const AnalysisSpec& spec);

void section_plane_lines(const Mesh& m, Vec3 n, float d, std::vector<Vec3>& lines);

bool write_mill_gcode(const std::string& path, const Mesh& mesh, const std::vector<struct HoleRow>& holes,
                      const MillSettings& s, std::string* err);
bool write_drawing_svg(const std::string& path, const Mesh& mesh, const std::vector<struct HoleRow>& holes,
                       const MassProps& mass, const char* title, const char* partno, const char* rev,
                       const char* material, const std::vector<std::string>& revs, std::string* err,
                       const std::vector<GdtFrame>& gdt = {}, const std::vector<DatumFeat>& datums = {});
bool write_drawing_dxf(const std::string& path, const Mesh& mesh, const std::vector<struct HoleRow>& holes,
                       const MassProps& mass, const char* title, const char* partno, const char* rev,
                       const char* material, const std::vector<std::string>& revs, std::string* err);

const char* analysis_mode_name(AnalysisMode m);
int estimate_dof(int n_free, int n_insert, int n_flush, int n_angle);

} // namespace ax
