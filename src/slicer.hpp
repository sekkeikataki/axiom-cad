#pragma once

#include "mesh.hpp"
#include <string>
#include <vector>

namespace ax {

enum class InfillPattern : std::uint8_t { Rectilinear, Grid, Triangles, Honeycomb, Gyroid, Cubic, Concentric };
enum class SeamMode : std::uint8_t { Nearest, Aligned, Rear };
enum class PathKind : std::uint8_t { External, Perimeter, Infill, Solid, Support, Skirt, Brim, Travel, Ironing, Bridge, Raft };
enum class GcodeFlavor : std::uint8_t { Marlin, Prusa, Klipper };
enum class WallOrder : std::uint8_t { InnerFirst, OuterFirst };
enum class PreviewColor : std::uint8_t { Type, Speed, Height };

struct PrinterPreset {
    const char* name;
    float bed_x, bed_y, bed_z;
    float nozzle;
    GcodeFlavor flavor;
    float retract;
};

struct FilamentPreset {
    const char* name;
    float nozzle_c, bed_c;
    float density;
    int fan;
    int fan_start;
    float retract;
    float pa;
    float min_layer_s;
    float max_vol;
};

struct QualityPreset {
    const char* name;
    float layer;
    int walls;
    float infill;
};

const PrinterPreset* printer_presets(int* n);
const FilamentPreset* filament_presets(int* n);
const QualityPreset* quality_presets(int* n);

struct PrintSettings {
    float bed_x = 250, bed_y = 210, bed_z = 220;
    float nozzle = 0.4f;
    float layer = 0.2f;
    float first_layer = 0.2f;
    float line_width = 0.42f;
    int walls = 2;
    int top_layers = 4;
    int bottom_layers = 4;
    float infill = 20; // %
    InfillPattern pattern = InfillPattern::Grid;
    float nozzle_c = 215, bed_c = 60;
    float print_mms = 50, travel_mms = 150, first_mms = 25, infill_mms = 70;
    float external_mms = 35, small_peri_mms = 20, small_peri_mm = 22, bridge_mms = 25, overhang_mms = 20;
    float retract_mm = 0.8f, retract_mms = 35, z_hop = 0.2f;
    float filament_d = 1.75f, flow = 1.f, density = 1.24f;
    int fan = 100;
    int fan_start_layer = 3;
    float min_layer_s = 10.f;
    int skirt = 1;
    float skirt_gap = 3.f;
    int brim = 0;
    bool supports = false;
    float support_angle = 45.f;
    float support_spacing = 4.f;
    int support_interface = 2;
    float support_xy = 0.6f;
    float support_z = 0.2f;
    bool support_bed_only = true;
    SeamMode seam = SeamMode::Aligned;
    WallOrder wall_order = WallOrder::InnerFirst;
    GcodeFlavor flavor = GcodeFlavor::Prusa;
    float elephant_foot = 0.15f;
    float wipe_mm = 2.f;
    bool ironing = false;
    float ironing_spacing = 0.2f;
    float ironing_flow = 0.15f;
    float ironing_mms = 20.f;
    float accel_print = 1500.f, accel_travel = 3000.f;
    float linear_advance = 0.05f;
    float max_vol = 12.f;
    float infill_overlap = 0.15f;
    int combine_infill = 1;
    bool connect_infill = true;
    bool monotonic = true;
    bool first_concentric = true;
    bool vase = false;
    bool adaptive = false;
    bool arc_fit = true;
    float scarf_mm = 4.f;
    float coast_mm = 0.4f;
    int raft = 0;
    float raft_gap = 0.2f;
    bool tree_supports = false;
    bool extra_wall_thin = true;
    std::vector<Vec2> paint_support;
    float paint_r = 8.f;
    bool auto_center = true;
    std::string printer_name = "Prusa MK4";
    std::string filament_name = "PLA";
    std::string quality_name = "0.20 mm Standard";
};

struct ToolPath {
    PathKind kind = PathKind::Perimeter;
    std::vector<Vec2> pts;
    bool closed = false;
    float speed = 0; // mm/s, 0 = derive from settings
};

struct SliceLayer {
    float z = 0;
    float height = 0.2f;
    float time_s = 0;
    std::vector<std::vector<Vec2>> loops;
    std::vector<ToolPath> paths;
};

struct SliceResult {
    bool ok = false;
    std::string error;
    PrintSettings settings;
    Vec3 shift; // applied to mesh before slice (to bed)
    std::vector<SliceLayer> layers;
    double filament_mm = 0;
    double filament_g = 0;
    double time_s = 0;
    int travels = 0;
    double slice_ms = 0;
    Aabb part_bounds;
};

std::vector<std::vector<Vec2>> mesh_loops_at(const Mesh& mesh, float z);
std::vector<std::vector<Vec2>> mesh_loops_plane(const Mesh& mesh, Vec3 n, float d, Vec3 u, Vec3 v);
std::vector<Vec2> offset_closed_loop(const std::vector<Vec2>& loop, float dist);
std::vector<std::pair<Vec2, Vec2>> hatch_in_loops(const std::vector<std::vector<Vec2>>& loops, float spacing,
                                                  float angle_deg);
float loop_area(const std::vector<Vec2>& loop);

SliceResult slice_mesh(const Mesh& mesh, const PrintSettings& s);
bool write_gcode(const std::string& path, const SliceResult& r, std::string* err);
void collect_preview(const SliceResult& r, int layer_from, int layer_to, bool travels, PreviewColor color,
                     std::vector<Vec3>& lines, std::vector<Vec3>& colors);
Vec3 path_color(PathKind k);
const char* path_kind_name(PathKind k);
const char* path_kind_gcode(PathKind k);
const char* infill_name(InfillPattern p);
const char* flavor_name(GcodeFlavor f);
const char* wall_order_name(WallOrder o);
const char* preview_color_name(PreviewColor c);
void apply_printer(PrintSettings& s, int index);
void apply_filament(PrintSettings& s, int index);
void apply_quality(PrintSettings& s, int index);
int find_printer(const char* name);
int find_filament(const char* name);

} // namespace ax
