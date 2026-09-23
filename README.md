# Axiom

A parametric solid modeler written in **C++20** for Arch Linux (and other Unix systems with OpenGL 3.3).

**Repository:** [github.com/sekkeikataki/axiom-cad](https://github.com/sekkeikataki/axiom-cad)

```bash
git clone https://github.com/sekkeikataki/axiom-cad.git
cd axiom-cad
```

Inventor, Fusion 360, and Creo are feature-based CAD systems with a history tree, workplanes, sketches, and boolean solids. Axiom is a from-scratch kernel that follows that model: no OpenCASCADE, no commercial geometry library, no garbage collector. The hot path is native C++ with a signed-distance CSG kernel and exact meshes for single primitives.

You work the same way as in those tools: constrained sketches, a feature timeline, extrude and revolve, holes, fillets, patterns, and New/Join/Cut/Intersect. Assemblies instance those parts with flush and insert mates, explode, and a bill of materials. A material library drives mass properties and a linear-elastic voxel stress map. Save a native `.axm` part or open / import / export the usual CAD mesh and drawing types. Imported geometry becomes an editable feature (move, scale, rotate, Join/Cut/Intersect). DXF and SVG become sketches you can extrude.

## Why C++

Runtime cost is the constraint that matters in a modeler: tessellation, ray hits, and the viewport all sit on the same frame. C++ gives direct control of layout, SIMD-friendly math, and zero-overhead GPU upload. Rust is in the same performance class; the existing graphics and CAD tooling on Linux is still overwhelmingly C/C++, so C++20 is the efficient choice here even though it costs more to write safely than a managed language.

## Build on Arch Linux

```bash
sudo pacman -S --needed base-devel cmake gcc glfw mesa
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/axiom
```

Or from the included `PKGBUILD`:

```bash
makepkg -si
axiom
```

## Build on other Linux

```bash
# Debian/Ubuntu
sudo apt install cmake g++ libglfw3-dev libgl1-mesa-dev pkg-config

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/axiom
```

Headless kernel check (no window):

```bash
./build/axiom --export-demo-stl /tmp/bracket.stl
./build/axiom --demo housing >/tmp/housing.axm
./build/axiom --demo flange >/tmp/flange.axm
./build/axiom --demo assembly >/tmp/assembly.axm
./build/axiom --demo handle >/tmp/handle.axm
./build/axiom --demo tray >/tmp/tray.axm
./build/axiom --demo spring >/tmp/spring.axm
./build/axiom --demo gear >/tmp/gear.axm
./build/axiom --demo ribbed >/tmp/ribbed.axm
./build/axiom --demo elbow >/tmp/elbow.axm
./build/axiom --demo coupling >/tmp/coupling.axm
./build/axiom --demo pillow >/tmp/pillow.axm
./build/axiom --demo vblock >/tmp/vblock.axm
./build/axiom --demo hanger >/tmp/hanger.axm
./build/axiom --analyze housing
./build/axiom --bench housing
./build/axiom --export-svg /tmp/housing.axm /tmp/housing.svg
./build/axiom --export /tmp/housing.axm /tmp/housing.step
./build/axiom --export /tmp/housing.stl /tmp/housing.obj
./build/axiom --slice examples/vblock.axm /tmp/vblock.gcode --layer 0.2 --infill 20
./build/axiom --drawing /tmp/housing.axm /tmp/housing-sheet.svg
./build/axiom --mill /tmp/housing.axm /tmp/housing.nc --tool 6
./build/axiom --mill /tmp/housing.axm /tmp/housing-face.nc --tool 6 --face
./build/axiom --topo /tmp/housing.axm
./build/axiom --rebind examples/box-fillet.axm
./build/axiom --fits examples/housing.axm
./build/axiom --inspect examples/housing.axm /tmp/housing-insp.csv
./build/axiom --stack examples/housing.axm
./build/axiom --dfm examples/housing.axm
./build/axiom --turn examples/flange.axm /tmp/flange-turn.nc
./build/axiom --analyze housing --modal
./build/axiom --analyze housing --thermal
```

## Detail and performance

The viewport does **not** show the export mesh. It sphere-traces the CSG signed-distance field, refines each hit, and draws Inventor-style silhouette/crease **contours**. The implicit pass is rendered at **3×** resolution with coverage-weighted edge samples, then linearly resolved so silhouettes stay anti-aliased instead of stair-stepped. Cylinders and fillets stay mathematically smooth at any zoom. A triangle mesh is still built only for STL/OBJ, mass, picking, and analysis — View → Mesh if you want to see it.

Boolean solids for export are tessellated with surface nets on CPU threads. Default mesh quality is **High (192)** (Fine **256**) for those file formats.

The tessellation combo on the QAT (Draft 96 / Medium 144 / High 192 / Fine 256) rebuilds the active body. Drop to Draft while laying out sketches.

## Materials, mass, and stress

Fifteen engineering materials ship in the library (aluminium 6061/7075, 304/316L, 4140, Ti-6Al-4V, brass, copper, cast iron, Inconel 718, ABS-GF, Nylon 6/6, POM, PEEK, alumina). Assign one in Properties.

Mass properties use tet-volume integration on the mesh: volume, area, mass, centre of mass, diagonal inertia about the COM, and stock cost.

**Analyze → Stress** (Shift+A) solves a linear-elastic occupancy lattice. Pick a **Fixture** face and a **Load** face (or leave them unset for the default low-Z / high-Z pair). Displacement is relaxed, strain is converted with isotropic Hooke’s law, and von Mises stress is painted on the mesh (blue → red). The safety factor is σy / σmax.

**Analyze → Modal** estimates the first-mode frequency from the same lattice (k = EA/L, f = (1/2π)√(k/m)). **Analyze → Thermal** solves a steady Laplace temperature field between the fixture (T_cold) and load (T_hot) faces.

**Analyze → Case 2** runs a second load (default lateral). **Compare** paints |σ₁ − σ₂|. von Mises now includes shear. Fixture/load face separation sets the characteristic length.

This is visualization-grade, not certified Nastran.

## Assemblies

The **Assemble** ribbon places instances of the built-in parts (housing, flange, pulley, bracket, handle, tray, spring, gear, ribbed, elbow). Each component has its own material, origin, and ground flag.

- **Insert mate** — concentric on Z (or X/Y): bolt-in-hole / shaft-in-bore.
- **Flush mate** — stack faces along an axis with an optional offset.
- **Grounded parts** — the mate solver holds grounded components and iterates 28 times.
- **DOF** — 6 × free − 3 × insert − 1 × flush − 1 × angle, shown on the Assemble ribbon.
- **Stack** — bolt + washer + nut on every hole in the hole table.
- **Interfere** — AABB prefilter, then triangle hits so flush mates that only touch are not clashes.
- **Explode** — pull components away from the assembly COM for documentation.
- **BOM** — grouped quantity, mass, and cost in Properties.

The red **A** application menu → Examples → Gearbox assembly is a housing (6061) with a 316L flange and a 4140 pulley, insert+flush mated on the boss.

Rebuilds hash the feature tree and skip tessellation when nothing geometric changed. Dirty parts autosave to `/tmp/axiom-autosave.axm` every 90 seconds.

## Modeling features

Beyond boxes, holes, fillets, and patterns:

| Feature | What it does |
| --- | --- |
| **Sweep** (`W`) | Pipe or profile along a finished sketch path |
| **Loft** (Shift+`E`) | Blend two finished sketch profiles |
| **Coil** (Shift+`W`) | Helical spring / thread body (major R, tube R, pitch, turns) |
| **Slot** (`K`) | Stadium slot — typically a cut |
| **Shell** (Shift+`H`) | Hollow the solid, opening one face |
| **Hole styles** | Simple, counterbore, or countersink in Properties |
| **Undo / Redo** | Ctrl+Z / Ctrl+Y (also on the QAT) |
| **Sketch arc** (`A`) | Three-click centre / start / end; chains with lines into closed profiles |
| **Draft / midplane extrude** | Taper angle and One-direction / Midplane in Properties (`depth, taper, 1`) |
| **Gear** (`G`) | Involute-ish spur gear — teeth, outer R, face width, bore |
| **Thread** (`T`) | External stud or internal tap (Cut + Internal) — major R, pitch, length |
| **Rib** (Shift+`R`) | Right-triangle web from a wall corner |
| **Drawing sheet** | Inspect → four-view 2D projection with overall dimensions |
| **Interference** | Mesh clash after AABB prefilter (flush mates that only touch are ignored) |
| **Driving dims** | Measure two faces; the distance writes a parameter that rebuilds the part |
| **Face / edge pick** | Select classifies a face (normal cluster) and crease edge on the implicit solid |
| **Tangent / concentric** | Sketch auto-constraint when a line kisses a circle or two circles share a centre |
| **Drawing sheet** | Inspect → four views, hole callouts, section A-A, revision table; SVG or DXF |
| **2.5D mill** | Face the grown outline, pocket inner loops, contour the true profile, G81 drills. Bind a picked face so XY follows that loop and Z is depth along its normal |
| **Local fillet** | Pick an edge, then Fillet/Chamfer — rolling-ball blend on that crease, chained around the half-edge loop (closes last-to-first). Hover previews the chain |
| **ISO 286 fits** | Hole/shaft classes (H7, g6, …) with ES/EI limits on the hole table and drawing callout |
| **GD&T** | Datums A/B/C and feature control frames (position, flatness, perpendicularity, …) on the sheet |
| **Tolerance stack** | Worst-case and RSS on driven dims + hole fits |
| **Inspection** | Measure holes from the SDF, pass/fail vs fit limits, CSV report |
| **DFM** | Wall, hole aspect, H7 ream, crease-count checks |
| **Lathe** | 2-axis face + OD rough/finish G-code (X = diameter) |
| **COM marker** | Viewport triad + label at the mass centre |
| **Work plane** | Offset construction plane; new sketches inherit its origin |
| **Copy / Scale / Move** | Ctrl+D, Shift+S, Shift+M — parametric feature edits |
| **Sketch fillet** | Convert rectangles to rounded profiles |
| **Construction** | Sketch entities that do not extrude |
| **Torus** (`U`) | Full, half, or quarter torus — elbows and O-rings |
| **Pipe** | Hollow sweep along a finished sketch path |
| **Helix** (Shift+`X`) | Helical tube or pipe (major R, tube R, pitch, turns) |
| **Path pattern** | Instance a feature along a sketch path |
| **Thicken** | Offset the solid outward |
| **Draft** | Taper the body along +Z |
| **Ellipse / spline** | Sketch an ellipse (centre + corner) or a quadratic spline |
| **User parameters** | Named values that drive `size.x` / depth (`PARAM` in the `.axm`) |
| **ISO holes** | M3–M16 tap / close / normal / loose clearance in Properties |
| **Angle mate** | Rotate a component about X/Y/Z by an offset angle |
| **Thickness / draft maps** | Analyze ribbon — paint wall thickness or draft vs +Z |
| **Groove** (`J`) | O-ring / retaining-ring torus cut on a shaft or bore |
| **Pocket** | Rectangular pocket with optional corner radius |
| **Boss** | Cylindrical boss; optional concentric hole |
| **Keyway** | Shaft keyseat box cut |
| **Hex** (Shift+`B`) | Hex prism or Allen socket (across-flats × height) |
| **Split** | Keep one side of a work plane |
| **Work axis / point** | Construction geometry drawn in the viewport |
| **Polygon / offset** | Regular N-gon sketch and offset of the last profile |
| **iProperties** | Title, part number, revision, author, description |
| **Named views** | Save / restore camera in Properties |
| **Projected area** | XY / XZ / YZ silhouette area in Properties |
| **Measure angle** | Third click after a distance reports the angle at B |
| **DXF export** | Top-view crease drawing (`drawing.dxf`) |
| **Viewport capture** | F11 writes `viewport.png` (PPM fallback) |
| **Text / engrave** (Shift+`T`) | Stick-font letters cut or joined into the solid |
| **ISO threads** | M3–M16 coarse table on Thread in Properties |
| **Sketch mirror** | Mirror the active sketch across X or Y |
| **Drawing title block** | Title, part number, rev, material, mass on the sheet |
| **Zebra** | Analyze ribbon — reflection stripes on the export mesh |
| **Capsule** (Shift+`J`) | Rounded pin / shaft (radius × length) |
| **Wedge** | Triangular prism — V-cuts and ramps (flip slope in Properties) |
| **L-angle / C-channel / I-beam** | Structural profiles on the Structure ribbon |
| **Dovetail** | Trapezoid slot cut (width, length, height, angle) |
| **Bolt / nut / washer** | Hex-head hardware with auto across-flats |
| **Hole table** | Inspect → Holes — callouts, CSV, drawing sheet |
| **Section plane** | X / Y / Z clip + section-area (mm²) in Properties |
| **Param equations** | `PAREXPR` — `Height*0.5` drives another parameter |
| **Configurations** | Named param sets (e.g. Standard / Wide on the V-block) |
| **Isolate** | F8 — rollback history to the selected feature |
| **Grid snap** | F9 — 5 mm default, increment in Properties |
| **Drawing SVG / BOM CSV** | Application menu or Inspect / Properties |

Examples: **Carrying handle** (sweep + slot + counterbores), **Shelled tray**, **Coil spring**, **Spur gear** (gear + hub + keyway + thread), **Ribbed bracket** (ribs + tapered D-pad + stud), **Pipe elbow** (quarter torus + flanges + ISO bolt circles, driven by `ElbowR` / `TubeR`), **Shaft coupling** (hub + flanges + O-ring grooves + keyway + hex socket + bolt circle, driven by `HubR` / `BoreR`), **Pillow block** (housing + bore groove + slots + ribs + engraved AXIOM, driven by `BoreR` / `BaseW`), **V-block** (90° wedge cuts, clamp slots, M8 hold-downs, `Width` / `Height` / `VDepth=Height*0.5`, Standard/Wide configs), **Shaft hanger** (C-channel + L-feet + capsule pin + washer/nut).

## Inventor-style UI

The desktop chrome follows Autodesk Inventor: a red **A** application button, a quick-access toolbar (New / Open / Save), and a pictorial ribbon with isometric command icons.

| Region | What it does |
| --- | --- |
| Red **A** | Application menu — New, Open, Save, export, examples |
| QAT | New / Open / Save / Undo / Redo, snap, quality, palette, help |
| Ribbon | Tabs: 3D Model, Structure, Sketch, Assemble, Inspect, Analyze, View — grouped Create / Modify / Detail / Pattern / Special |
| Model browser | Feature history, filter, hide / isolate / copy context menu |
| Viewport HUD | Live box/radius dims, overall AABB, hover tooltip, mini-toolbar on selection |
| Status bar | Active tool, command line, snap, cursor, triangle count, rebuild ms, FPS |
| ViewCube | TOP / FRONT / RIGHT faces; double-click for isometric |
| Nav bar | Home, Fit, orthographic views, shaded / wire, section |

Icons are a generated atlas (`src/icons.cpp`), not photos, so they stay sharp at 40×40 on the ribbon and 16×16 in the browser.

## Controls

| Action | Input |
| --- | --- |
| Orbit | Middle-drag or Alt+left-drag |
| Pan | Shift+middle-drag |
| Zoom | Scroll |
| Fit | `F` |
| Front / Top / Right / Iso | `1` `2` `3` `7` |
| Box / Cylinder / Sphere / Cone / Hole | `B` `C` `S`  Shift+`C`  `H` |
| Sketch rectangle / circle / line / arc | `R` `O` `L` `A` |
| Extrude / Revolve | `E` / `V` |
| Gear / Thread / Rib | `G` / `T` / Shift+`R` |
| Scale / Move / Copy | Shift+`S` / Shift+`M` / `Ctrl+D` |
| Snap / Isolate / Repeat | `F9` / `F8` / Space |
| Fillet / Chamfer | `Q` / Shift+`Q` |
| Rect / circular pattern | `P` / Shift+`P` |
| Stress analysis | Shift+`A` |
| Command palette / section | `Ctrl+K` / `X` |
| Groove / Hex | `J` / Shift+`B` |
| Capture viewport | `F11` |
| Measure | `M` |
| Join / Cut while finishing a tool | Shift / Ctrl |
| Dimensions | Command bar, e.g. `40, 30, 12` then Enter |
| Cancel | Esc |

Units are millimetres. The world is **Z-up** (XY is the top plane), same as Inventor, Fusion, and Creo.

## File format

`.axm` is a versioned text part file: sketches, features, boolean operations, material, components, mates, explode, rollback, and embedded `MESHDATA` for imported solids.

Open, Import (Ctrl+I), Export, or drop a file on the window. The extension picks the translator:

| Family | Extensions | How Axiom treats it |
| --- | --- |
| Native | `.axm` | Full parametric history |
| Mesh | `.stl` `.obj` `.ply` `.off` `.amf` `.3mf` `.gltf` `.glb` `.wrl` `.x3d` `.dae` `.json` | **Import** feature — origin, rotation, scale, Join/Cut |
| CAD exchange | `.step` `.stp` `.iges` `.igs` | Tessellated write; CARTESIAN_POINT / POLY_LOOP / IGES 110 read |
| Drawing | `.dxf` `.svg` | Editable sketch (auto-extruded when opened alone); DXF also reads `3DFACE` |
| Points / tables | `.xyz` `.asc` `.pts` `.csv` | Point fan or hole table (`x y z dia`) |
| Heightmap | `.ppm` `.pgm` | Grid mesh from brightness |

STEP/IGES are tessellated (no B-rep kernel). 3MF write is an uncompressed ZIP; zlib-linked builds also read deflated packages.

## 3D printing

The **Print** ribbon is an in-app FFF slicer in the same vein as PrusaSlicer / OrcaSlicer: printer and filament presets, layer height, walls, infill patterns, supports, brim/skirt, retraction, cooling, and a layer preview on the part. Slice writes Marlin, Prusa, or Klipper G-code.

| Control | What it does |
| --- | --- |
| Printer | Prusa MK4 / MK4S / MK3S+ / Mini+ / XL / CORE One, Voron 2.4 / 0.2, Bambu P1S / X1C / A1, Ender 3, Creality K1 |
| Filament | PLA, PETG, ABS, ASA, TPU, Nylon — temps, fan, retract, pressure advance |
| Quality | Fast 0.28 · Standard 0.20 · Quality 0.16 · Detail 0.12 · Extra fine 0.10 · Structural |
| Infill | Rectilinear, grid, triangles, honeycomb, gyroid, cubic, concentric — connected zigzag, optional monotonic solids |
| Walls | Inner-first (Prusa default) or outer-first; nearest-neighbour travel order |
| First layer | Elephant-foot inset, concentric solid option, outline brim, optional raft |
| Speeds | External / small-perimeter / overhang / bridge, volumetric mm³/s cap |
| Cooling | Fan starts after N layers; minimum layer time (slow or `G4`) |
| Supports | Overhang grid or **tree** trunks, XY/Z gap, interface, “bed only”, **paint** (Ctrl+click) |
| Thin walls | Extra perimeter in regions narrower than ~18 line-widths (Arachne-ish) |
| Modes | Spiral vase, adaptive layer height, combine infill every N, ironing |
| G-code | Arc fitting (`G2`/`G3`), scarf seam, coast, wipe + Z-hop, `M73` |
| Preview | Colour by type, speed, or height; layer time in the HUD (Shift+scroll) |

Toolpaths are coloured: orange external walls, gold perimeters, blue infill, green solid top/bottom, purple support, pink ironing, navy bridges. Estimates report filament grams and print time.

```bash
axiom --slice examples/vblock.axm /tmp/vblock.gcode --layer 0.2 --infill 20
axiom --slice examples/hanger.axm /tmp/hanger.gcode --printer Voron --filament PETG --flavor klipper --supports --ironing
axiom --slice examples/vblock.axm /tmp/vase.gcode --vase
axiom --slice examples/hanger.axm /tmp/adapt.gcode --adaptive --raft 2
axiom --slice examples/hanger.axm /tmp/tree.gcode --supports --tree
axiom --drawing examples/housing.axm /tmp/housing-sheet.svg
axiom --drawing examples/housing.axm /tmp/housing-sheet.dxf
axiom --mill examples/housing.axm /tmp/housing.nc --tool 6 --stepover 0.4
axiom --mill examples/housing.axm /tmp/housing-face.nc --tool 6 --face
axiom --topo examples/housing.axm
```

Print → **Bed** frames the plate from above. Ctrl+click the preview to paint support enforcers.

## Drawings and mill

Inspect → **Drawing** is an associative four-view sheet (top / front / right / iso) with overall dims, hole callouts, section A-A through mid-Y, title block, and a revision table. **Sheet** writes `drawing.svg`; the sheet window also writes DXF. Stamp a revision from iProperties or the sheet.

Inspect → **Mill** writes LinuxCNC/Fanuc-flavoured 2.5D: face the stock down to the part top, pocket inner loops, finish-contour, then G81 the hole table. Coordinates are fixed 3-decimal millimetres (no scientific notation). Tick **Follow picked face** (or pass `--face`) and the toolpaths sit on that face’s half-edge outline instead of a Z-slice of the AABB.

Inspect → **Inspect** is the shop-floor pack: ISO 286 hole/shaft limits, datum A/B/C plus feature-control frames, a worst-case + RSS stack, SDF-measured inspection with pass/fail, and a DFM pass (thin walls, deep holes, H7 ream). **Turn** writes 2-axis lathe G-code (face + OD, X as diameter). Drawings print `Ø16 H7` and FCF boxes.

Local fillet/chamfer walks the crease loop from the picked edge (half-edge twins + dihedral creases) and blends every segment, including the last-to-first edge on a closed loop. Hover Fillet or Chamfer to preview the chain; uncheck **Chain around loop** in Properties to keep only the seed. The seed crease is stored as `EDGEID` (the two face keys plus a slot, plus a loop flag) so a size edit re-resolves the chain on the solid *before* the blend — XYZ is only a cache (`EDGECHAIN`). Inspect → Mill can bind a face the same way (`MILLFACE` keeps a face key).

```

## Architecture

- `src/math.hpp` — vectors, matrices, rays, AABBs
- `src/io.*` — format detection and mesh/sketch/CAD translators
- `src/slicer.*` — layer slice, perimeters, infill, tree/grid/paint supports, G-code
- `src/engineer.*` — face/edge pick, driven dims, mesh clash, 2.5D mill, associative drawings, FEA cases
- `src/inspect.*` — ISO 286, GD&T, stack-up, DFM, inspection CSV, lathe G-code
- `src/mesh.*` — SDF primitives, CSG, cached inverses, threaded surface nets, exact tessellation, half-edge weld/crease/face keys
- `src/material.*` — library, mass properties, voxel FEA
- `src/solve.*` — 2D sketch constraint solver (incl. tangent / concentric)
- `src/document.*` — parametric history, assemblies, serialize/export
- `src/render.*` — OpenGL 3.3 viewport, studio lighting, von Mises colormap
- `src/app.*` — Inventor-style QAT, pictorial ribbon, icon browser, timeline, palette
- `src/icons.*` — isometric command atlas (CPU rasterizer → GL texture)

Windowing is GLFW. The overlay UI is Dear ImGui. Both sit outside the kernel; replacing them does not touch geometry.
