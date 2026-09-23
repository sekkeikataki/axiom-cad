#pragma once

#include <cstdint>
#include <glad/gl.h>

namespace ax {

enum class Icon : int {
    New = 0,
    Open,
    Save,
    Select,
    Box,
    Cylinder,
    Sphere,
    Cone,
    Hole,
    Extrude,
    Revolve,
    Fillet,
    Chamfer,
    RectPattern,
    CircPattern,
    Mirror,
    SketchRect,
    SketchCircle,
    SketchLine,
    Measure,
    Section,
    Place,
    MateInsert,
    MateFlush,
    Explode,
    Stress,
    Material,
    Fit,
    Home,
    Top,
    Front,
    Right,
    Shaded,
    Wire,
    PlaneXY,
    PlaneXZ,
    PlaneYZ,
    Origin,
    Feature,
    Sketch,
    Component,
    Ground,
    Palette,
    Help,
    Sweep,
    Shell,
    Coil,
    Slot,
    Loft,
    Undo,
    Redo,
    SketchArc,
    Gear,
    Thread,
    Rib,
    Drawing,
    Interfere,
    Com,
    Copy,
    Isolate,
    Snap,
    Scale,
    WorkPlane,
    Construction,
    Torus,
    Pipe,
    Helix,
    PathPattern,
    Thicken,
    Draft,
    Ellipse,
    Spline,
    Params,
    Thickness,
    Groove,
    Pocket,
    Boss,
    Keyway,
    Hex,
    Split,
    WorkAxis,
    WorkPoint,
    Polygon,
    Offset,
    Capture,
    Dxf,
    Text,
    Zebra,
    Capsule,
    Wedge,
    Angle,
    Channel,
    IBeam,
    Dovetail,
    Bolt,
    Nut,
    Washer,
    Holes,
    Print,
    Slice,
    Infill,
    Support,
    Gcode,
    COUNT
};

struct IconAtlas {
    GLuint tex = 0;
    int cell = 64;
    int cols = 8;
    int rows = 6;
    int tw = 0, th = 0;
    bool init();
    void shutdown();
    void uv(Icon id, float uv0[2], float uv1[2]) const;
};

IconAtlas& icons();
Icon icon_for_feature(int kind);
Icon icon_for_tool(int tool);

} // namespace ax
