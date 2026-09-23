#pragma once

#include "icons.hpp"
#include "render.hpp"
#include "slicer.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

namespace ax {

enum class FileAction { Open, Save, Import, Export };

enum class Tool {
    Select,
    Box,
    Cylinder,
    Sphere,
    Cone,
    SketchRect,
    SketchCircle,
    SketchLine,
    Extrude,
    Revolve,
    Fillet,
    Chamfer,
    Hole,
    RectPattern,
    CircPattern,
    Mirror,
    Measure,
    Sweep,
    Shell,
    Coil,
    Slot,
    Loft,
    SketchArc,
    Gear,
    Thread,
    Rib,
    SketchFillet,
    WorkPlane,
    Scale,
    Move,
    Copy,
    Torus,
    Pipe,
    Helix,
    PathPattern,
    Thicken,
    Draft,
    SketchEllipse,
    SketchSpline,
    Groove,
    Pocket,
    Boss,
    Keyway,
    Hex,
    Split,
    WorkAxis,
    WorkPoint,
    SketchPolygon,
    SketchOffset,
    Text,
    SketchMirror,
    Capsule,
    Wedge,
    Angle,
    Channel,
    IBeam,
    Dovetail,
    Bolt,
    Nut,
    Washer
};

struct App {
    GLFWwindow* win = nullptr;
    Renderer renderer;
    Camera camera;
    Document doc;
    Tool tool = Tool::Select;
    int selected = 0;
    int hover = 0;
    int sketching_id = 0;
    int step = 0;
    Vec3 pts[4]{};
    Vec2 spts[4]{};
    Mesh preview;
    std::string status = "Select a tool or start a solid primitive.";
    std::string error;
    std::string cmd;
    std::string file_modal_path = "part.axm";
    bool file_modal_open = false;
    FileAction file_action = FileAction::Open;
    bool show_help = false;
    bool show_palette = false;
    bool show_app_menu = false;
    bool show_drawing = false;
    bool show_com = false;
    bool show_interfere = false;
    bool show_holes = false;
    bool show_inspect = false;
    int section_axis = 0; // 0=X 1=Y 2=Z
    std::vector<Clash> clashes;
    bool want_quit = false;
    bool viewport_hovered = false;
    int ribbon = 0;
    int selected_comp = 0;
    int mate_pick_a = 0;
    int mate_kind = 0;
    Vec2 measure_a, measure_b;
    bool measure_has_a = false;
    bool measure_has_b = false;
    Vec3 measure_wa, measure_wb, measure_wc;
    bool measure_has_c = false;
    std::vector<std::string> undo_stack;
    std::vector<std::string> redo_stack;
    std::vector<std::string> recent;
    Tool last_tool = Tool::Box;
    bool snap = true;
    float snap_inc = 5.f;
    bool construction = false;
    bool show_dims = true;
    bool isolating = false;
    int isolate_rollback = -1;
    float fps = 0;
    Vec3 cursor_w{};
    bool cursor_valid = false;
    double last_autosave = 0;
    PrintSettings print;
    SliceResult slice;
    bool show_print_preview = false;
    bool show_travels = false;
    bool preview_single = true;
    int preview_layer = 0;
    PreviewColor preview_color = PreviewColor::Type;
    int printer_sel = 0;
    int filament_sel = 0;
    int quality_sel = 1;
    FaceRef pick_face;
    FaceRef pick_face_b;
    EdgeRef pick_edge;
    std::vector<Vec3> hover_chain;
    bool hover_loop = false;

    bool init();
    void shutdown();
    void run();

    void apply_theme();
    void ui();
    void qat();
    void toolbar();
    void browser();
    void nav_bar(float vx, float vy, float vw, float vh);
    void properties();
    void command_bar();
    void timeline();
    void palette();
    void view_cube(float vx, float vy, float vw, float vh);
    void viewport();
    void handle_shortcuts();
    void on_viewport_click(float x, float y, int button);
    void on_viewport_drag(float dx, float dy, int button, bool shift, bool alt);
    void cancel_tool();
    void commit_numeric();
    void begin_tool(Tool t);
    void update_preview();
    void finish_primitive();
    void run_analysis();
    void place_part(const char* src);
    Ray viewport_ray(float x, float y);
    bool hit_workplane(const Ray& ray, int plane, Vec3 origin, Vec3& hit);
    void new_doc();
    void load_example(const char* name);
    void set_status(const char* s);
    void snapshot();
    void undo();
    void redo();
    void apply_feature(Feature f);
    void drawing_sheet();
    void interfere_panel();
    void hole_table_panel();
    void inspect_panel();
    void sync_section();
    void status_bar();
    void mini_toolbar(float vx, float vy);
    void remember_path(const std::string& p);
    void load_recent();
    Vec3 snapped(Vec3 p) const;
    void copy_selected();
    void isolate_selected();
    void end_isolate();
    void apply_quality(int q);
    bool capture_viewport(const std::string& file, std::string* err);
    bool capture_png(const std::string& file, std::string* err);
    void begin_file(FileAction a, const char* suggest = nullptr);
    void open_dropped(const std::string& path);
    void run_slice();
    void print_panel();
};

} // namespace ax
