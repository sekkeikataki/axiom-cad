# Contributing to Axiom

This is the public tree. Clone it, build it, and keep going from here — you do not need the Cloud Agent workspace.

```bash
git clone https://github.com/sekkeikataki/axiom-cad.git
cd axiom-cad
```

## Build

Arch:

```bash
sudo pacman -S --needed base-devel cmake gcc glfw mesa
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/axiom
```

Debian / Ubuntu:

```bash
sudo apt install cmake g++ libglfw3-dev libgl1-mesa-dev pkg-config zlib1g-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

If `third_party/imgui/imgui.cpp` is not in the clone, CMake downloads Dear ImGui v1.91.8 (or run `scripts/fetch-deps.sh`). GLAD is vendored under `third_party/glad/` and must be present.

Headless checks (no window):

```bash
./build/axiom --rebind examples/box-fillet.axm
./build/axiom --topo examples/box-fillet.axm
./build/axiom --fits examples/housing.axm
./build/axiom --inspect examples/housing.axm /tmp/housing-insp.csv
./build/axiom --stack examples/housing.axm
./build/axiom --dfm examples/housing.axm
./build/axiom --mill examples/housing.axm /tmp/housing.nc --tool 6 --face
./build/axiom --turn examples/flange.axm /tmp/flange-turn.nc
./build/axiom --slice examples/vblock.axm /tmp/vblock.gcode --layer 0.2 --infill 20
```

## Layout

| Path | Role |
| --- | --- |
| `src/math.hpp` | vectors, matrices, rays, AABBs |
| `src/mesh.*` | SDF CSG, surface nets, half-edge crease / face keys |
| `src/document.*` | feature history, `.axm`, rebuild, assemblies |
| `src/solve.*` | 2D sketch constraints |
| `src/engineer.*` | face/edge pick, mill, drawings, FEA cases |
| `src/inspect.*` | ISO 286, GD&T, stack, DFM, lathe, inspection CSV |
| `src/slicer.*` | FFF slice → G-code |
| `src/io.*` | import / export translators |
| `src/material.*` | library, mass, voxel stress |
| `src/render.*` | OpenGL 3.3 viewport |
| `src/app.*` | Inventor-style ribbon / browser |
| `src/trace_shader.hpp` | GPU SDF sphere-trace |
| `examples/*.axm` | parametric parts (housing, flange, box-fillet, …) |
| `ROADMAP.md` | honest kernel limits and the next real jumps |

Units are millimetres. The world is **Z-up**.

## Rules that keep the kernel honest

- There is no B-rep / OpenCASCADE. Do not pretend fillets are NURBS face blends.
- Local fillet/chamfer must resolve `EDGEID` on the **predecessor** mesh (the solid before that EdgeBlend).
- Closed crease loops must emit the last→first segment (`edge.loop` + `chain.front()`).
- Mill `--face` binds a `FaceKey`; G-code numbers are fixed 3 decimal millimetres.
- Inspection vs H7 is mesh-grade, not a CMM. FEA is a visualization lattice, not Nastran.

Read `ROADMAP.md` before adding another SDF boolean. The grade-raising work is topology, not more primitives.

## License

MIT. See `LICENSE`.
