#pragma once

#include "engineer.hpp"
#include "inspect.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "solve.hpp"
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ax {

enum class FeatureKind : std::uint8_t {
    Box,
    Sphere,
    Cylinder,
    Cone,
    Sketch,
    Extrude,
    Revolve,
    Fillet,
    Chamfer,
    Hole,
    RectPattern,
    CircPattern,
    Mirror,
    Sweep,
    Shell,
    Coil,
    Slot,
    Loft,
    Rib,
    Thread,
    Gear,
    WorkPlane,
    Torus,
    Pipe,
    Helix,
    PathPattern,
    Thicken,
    Draft,
    Groove,
    Pocket,
    Boss,
    Keyway,
    Hex,
    Split,
    WorkAxis,
    WorkPoint,
    Text,
    Capsule,
    Wedge,
    Angle,
    Channel,
    IBeam,
    Dovetail,
    Bolt,
    Nut,
    Washer,
    Import
};

struct SketchEnt {
    enum class Type : std::uint8_t { Line, Rect, Circle, Arc, Ellipse, Spline, Polygon } type = Type::Line;
    Vec2 a, b; // line endpoints / rect corners / arc center+start
    float r = 0; // circle radius, or arc sweep degrees
    bool construction = false;
    int n = 6; // polygon sides
};

struct Sketch {
    int id = 0;
    std::string name;
    int plane = 0; // 0=XY, 1=XZ, 2=YZ
    Vec3 origin;
    std::vector<SketchEnt> ents;
    std::vector<SkCon> cons;
    bool finished = false;

    std::vector<std::vector<Vec2>> profiles() const;
    std::vector<Vec2> polyline() const;
    void solve();
};

struct Feature {
    int id = 0;
    std::string name;
    FeatureKind kind = FeatureKind::Box;
    bool suppressed = false;
    BoolOp op = BoolOp::NewBody;
    Vec3 origin;
    Vec3 euler_deg;
    Vec3 size{40, 30, 20};
    int sketch_id = 0;
    float depth = 10;
    int plane = 0;
    float radius = 0;
    int count = 1;
    int count2 = 1;
    float spacing = 20;
    float spacing2 = 20;
    float angle = 360;
    int source_id = 0;
    int axis = 2;
    int style = 0; // hole 0/1/2; extrude 1=midplane; thread 1=internal; torus 1=90 2=180
    bool hidden = false;
    int p_size = 0;  // user parameter id driving size.x
    int p_depth = 0; // user parameter id driving depth / size.y
    std::string text; // engraved / embossed string, or import source path
    std::string fit;  // ISO 286 class, e.g. H7 / g6
    Mesh import_mesh; // FeatureKind::Import — editable source triangles
    std::vector<Vec3> chain; // crease polyline for local fillet / chamfer
    CreaseId edge;          // persistent seed crease (faces + slot)
};

struct Param {
    int id = 0;
    std::string name;
    float value = 0;
    std::string expr; // optional: BoreR*2+4 — names refer to other params
};

struct Config {
    std::string name;
    std::vector<std::pair<int, float>> values; // param id → value
};

struct HoleRow {
    int id = 0;
    std::string name;
    Vec3 origin;
    float dia = 0;
    float depth = 0;
    const char* type = "Through";
    std::string fit;
    float fit_min = 0, fit_max = 0;
};

struct IsoHole {
    const char* name;
    float tap;
    float close;
    float normal;
    float loose;
};

const IsoHole* iso_hole_table(int* n);

struct IsoThread {
    const char* name;
    float major; // diameter mm
    float pitch;
};

const IsoThread* iso_thread_table(int* n);

enum class MateKind : std::uint8_t { Flush, Insert, Offset, Coincident, Angle };

struct Component {
    int id = 0;
    std::string name;
    std::string source; // bracket | pulley | housing | flange
    int material_id = 0;
    Vec3 origin;
    Vec3 euler_deg;
    bool grounded = false;
    bool suppressed = false;
    Mesh mesh;
    MassProps mass;
};

struct Mate {
    int id = 0;
    MateKind kind = MateKind::Flush;
    int a = 0, b = 0;
    int axis = 2; // 0=X 1=Y 2=Z
    float offset = 0;
    bool suppressed = false;
};

struct BomRow {
    std::string name;
    std::string material;
    int qty = 0;
    double mass_g = 0;
    double cost_usd = 0;
};

struct Clash {
    int a = 0, b = 0;
    std::string name_a, name_b;
    float overlap_mm3 = 0;
};

struct Document {
    std::string path;
    bool dirty = false;
    int next_id = 1;
    int rollback = -1; // -1 = all features
    std::vector<Feature> features;
    std::vector<Sketch> sketches;
    std::vector<Param> params;
    std::vector<Component> components;
    std::vector<Mate> mates;
    Mesh part; // unassembled solid
    Mesh body;
    SdfScene scene;
    int quality = 192;
    int material_id = 0;
    float explode = 0;
    float analysis_load_n = 25000;
    int analysis_grid = 28;
    std::string title;
    std::string part_number;
    std::string revision = "A";
    std::string author;
    std::string description;
    struct NamedView {
        std::string name;
        float yaw = 0, pitch = 0, distance = 160;
        Vec3 target;
        bool ortho = false;
    };
    std::vector<NamedView> views;
    std::vector<Config> configs;
    std::vector<DrivenDim> dims;
    std::vector<std::string> rev_log;
    FaceRef fix_face;
    FaceRef load_face;
    AnalysisSpec analysis;
    AnalysisSpec analysis_b{.load_dir = {1, 0, 0}, .force = 25000};
    int analysis_slot = 0; // 0 primary, 1 second, 2 compare
    MillSettings mill;
    TurnSettings turn;
    std::vector<GdtFrame> gdt;
    std::vector<DatumFeat> datums;
    MassProps mass;
    StressResult stress;
    bool analysis_painted = false;
    double last_rebuild_ms = 0;
    std::uint64_t rev = 0;
    std::uint64_t geom_key = 0;

    Feature* find_feature(int id);
    const Feature* find_feature(int id) const;
    Sketch* find_sketch(int id);
    const Sketch* find_sketch(int id) const;

    Feature& add_feature(Feature f);
    Sketch& add_sketch(Sketch s);
    bool remove_feature(int id);
    std::string unique_name(const std::string& prefix) const;

    void rebuild(bool force = false);
    std::uint64_t geometry_hash() const;
    SdfPrim feature_to_prim(const Feature& f) const;
    void compose_assembly();
    void solve_mates();
    int place_component(const char* source, int mat_id = 0);
    Component* find_component(int id);
    const Component* find_component(int id) const;
    Mate& add_mate(Mate m);
    std::vector<BomRow> bom() const;
    std::vector<Clash> interference(float shrink_mm = 0.75f) const;
    int dof() const;
    int stack_fasteners();
    void apply_driven_dims();
    bool export_drawing(const std::string& file, std::string* err) const;
    bool export_mill(const std::string& file, std::string* err) const;
    StressResult run_analysis();
    bool paint_thickness(float max_mm = 12.f);
    bool paint_draft(Vec3 pull = {0, 0, 1});
    bool paint_zebra(int bands = 10);
    Param* find_param(int id);
    const Param* find_param(int id) const;
    Param& add_param(const char* name, float value);
    float driven(const Feature& f, int which, float fallback) const;
    void eval_params();
    bool apply_config(const std::string& name);
    std::vector<HoleRow> hole_table() const;
    int ensure_datums();
    int suggest_gdt();
    std::vector<InspRow> inspect() const;
    std::vector<DfmIssue> dfm() const;
    StackResult stack() const;
    bool export_inspect(const std::string& file, std::string* err) const;
    bool export_dfm(const std::string& file, std::string* err) const;
    bool export_turn(const std::string& file, std::string* err) const;
    float section_area(Vec3 n, float d, int res = 48) const;
    bool export_holes_csv(const std::string& file, std::string* err) const;
    static Document part_from_source(const char* source);

    std::string serialize() const;
    static std::optional<Document> deserialize(const std::string& text, std::string* err);
    bool save(const std::string& file, std::string* err) const;
    static std::optional<Document> load(const std::string& file, std::string* err);
    bool export_stl(const std::string& file, std::string* err) const;
    bool export_obj(const std::string& file, std::string* err) const;
    bool export_svg(const std::string& file, std::string* err) const;
    bool export_dxf(const std::string& file, std::string* err) const;
    bool export_bom_csv(const std::string& file, std::string* err) const;
    bool open_any(const std::string& file, std::string* err);
    bool import_file(const std::string& file, std::string* err);
    bool export_any(const std::string& file, std::string* err) const;
    bool export_gcode(const std::string& file, std::string* err) const;
    bool has_imports() const;
    bool copy_feature(int id, Vec3 offset, Feature* out = nullptr);
    bool fillet_sketch(int sketch_id, float radius);
    bool offset_sketch(int sketch_id, float dist);
    bool mirror_sketch(int sketch_id, int axis); // 0=X 1=Y
    Vec3 projected_area() const; // XY, XZ, YZ mm²

    static Document demo_bracket();
    static Document demo_pulley();
    static Document demo_housing();
    static Document demo_flange();
    static Document demo_assembly();
    static Document demo_handle();
    static Document demo_tray();
    static Document demo_spring();
    static Document demo_gear();
    static Document demo_ribbed();
    static Document demo_elbow();
    static Document demo_coupling();
    static Document demo_pillow();
    static Document demo_vblock();
    static Document demo_hanger();
};

Plane sketch_plane(int plane, Vec3 origin);
void plane_basis(int plane, Vec3& u, Vec3& v, Vec3& n);
Vec3 plane_from_local(int plane, Vec3 origin, Vec2 q);
Vec2 plane_to_local(int plane, Vec3 origin, Vec3 p);
const char* kind_name(FeatureKind k);
std::vector<Vec2> sketch_arc_points(const SketchEnt& e, int segs = 48);
std::vector<Vec2> sketch_ellipse_points(const SketchEnt& e, int segs = 64);
std::vector<Vec2> sketch_spline_points(const SketchEnt& e, int segs = 24);
std::vector<Vec2> sketch_polygon_points(const SketchEnt& e);

} // namespace ax
