# Roadmap — continue here

Axiom is a from-scratch native C++ parametric CAD (not a web app). This file is the honest state of the kernel so the next session does not rediscover it.

## What is already in the tree

- Feature history, `.axm`, Inventor-style desktop UI (GLFW + Dear ImGui, OpenGL 3.3)
- SDF CSG + surface nets for export; GPU sphere-trace viewport with contours
- Half-edge weld / crease / face keys; local fillet/chamfer with persistent `EDGEID` (survives height/width rebuilds)
- Closed crease loops blend last→first; hover chain preview; seed-only vs chain
- 2.5D mill (face / pocket / contour / G81) and 2-axis lathe (face + OD, X = diameter)
- In-app FFF slicer, multi-format I/O, assemblies + mates, voxel FEA (static / modal / thermal / case 2)
- ISO 286 fits, GD&T frames, tolerance stack, DFM, inspection CSV
- Examples under `examples/` (housing has H7/H8); `PKGBUILD` for Arch

## Kernel honesty (do not paper over)

There is still **no B-rep / OpenCASCADE**. Fillets are rolling-ball SDF blends on crease polylines, not NURBS face–face blends. Inspection vs H7 is **mesh-grade**, not a CMM. FEA is a visualization lattice, not Nastran.

## Next jumps that actually raise the grade

1. **Persistent edge id on more features** — chamfer/fillet already rebind; hole axes and mill faces should use the same key scheme after every body-changing feature.
2. **True B-rep or a topology kernel** — if the goal is Inventor/Creo parity, this is the real cliff. Do not add more SDF boolean toys first.
3. **Variable / hold-line fillet** — radius along the chain, not one number.
4. **Lathe ID + thread + groove** — OD/face exist; bore and API/ISO thread cycles do not.
5. **Drawing associativity** — FCF and fit callouts should move with the view scale; DXF should get the same FCF boxes as SVG.
6. **Assembly mates that solve** — flush/insert exist; tangent and angle still need a real constraint iteration with over-constraint detection.
7. **STEP read that becomes features** — import is a mesh feature today.

## Verify before you change fillet/mill

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/axiom --rebind examples/box-fillet.axm    # must print rebind ok
./build/axiom --fits examples/housing.axm
./build/axiom --inspect examples/housing.axm /tmp/insp.csv
./build/axiom --turn examples/flange.axm /tmp/flange-turn.nc
```

Units are millimetres. World is **Z-up**.
