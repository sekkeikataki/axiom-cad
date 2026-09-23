#include "app.hpp"
#include "io.hpp"
#include "slicer.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    if (argc >= 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)) {
        std::printf("Axiom — parametric solid CAD\n\n"
                    "  axiom                  interactive modeler\n"
                    "  axiom FILE             open .axm or any supported mesh/drawing\n"
                    "  axiom --demo [bracket|pulley|housing|flange|assembly|handle|tray|spring|gear|ribbed|elbow|coupling|pillow|vblock|hanger]\n"
                    "  axiom --analyze [housing|bracket|flange] [--static|--modal|--thermal|--case2]\n"
                    "  axiom --export IN OUT  write OUT; extension selects STL/OBJ/PLY/OFF/AMF/3MF/glTF/GLB/WRL/X3D/DAE/STEP/IGES/DXF/SVG/JSON/XYZ/CSV\n"
                    "  axiom --export-stl IN.axm OUT.stl\n"
                    "  axiom --export-svg IN.axm OUT.svg\n"
                    "  axiom --export-dxf IN.axm OUT.dxf\n"
                    "  axiom --export-demo-stl OUT.stl\n"
                    "  axiom --slice IN OUT.gcode   slice to Marlin/Prusa/Klipper G-code\n"
                    "           [--layer] [--infill] [--walls] [--supports] [--tree] [--ironing] [--vase]\n"
                    "           [--adaptive] [--raft N] [--printer NAME] [--filament NAME]\n"
                    "           [--flavor marlin|prusa|klipper]\n"
                    "  axiom --drawing IN OUT.svg|.dxf   associative 4-view drawing (holes, section, revs)\n"
                    "  axiom --mill IN OUT.nc            2.5D face / pocket / contour / G81\n"
                    "           [--tool D] [--stepover F] [--stepdown F] [--face]\n"
                    "  axiom --topo IN                   half-edge weld / crease / chain report\n"
                    "  axiom --rebind IN                 resize the first box and re-resolve local fillets\n"
                    "  axiom --inspect IN [OUT.csv]      ISO 286 + GD&T + measured inspection\n"
                    "  axiom --dfm IN [OUT.txt]          manufacturability report\n"
                    "  axiom --turn IN OUT.nc            2-axis lathe (face + OD)\n"
                    "  axiom --fits IN                   hole table with ISO 286 limits\n"
                    "  axiom --stack IN                  worst-case + RSS tolerance stack\n"
                    "  axiom --bench [housing|gear|ribbed|coupling]\n\n"
                    "  %s\n",
                    ax::supported_formats_help().c_str());
        return 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--slice") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        ax::PrintSettings s;
        for (int i = 4; i < argc; ++i) {
            if (std::strcmp(argv[i], "--layer") == 0 && i + 1 < argc) s.layer = s.first_layer = (float)std::atof(argv[++i]);
            else if (std::strcmp(argv[i], "--infill") == 0 && i + 1 < argc) s.infill = (float)std::atof(argv[++i]);
            else if (std::strcmp(argv[i], "--walls") == 0 && i + 1 < argc) s.walls = std::atoi(argv[++i]);
            else if (std::strcmp(argv[i], "--supports") == 0) s.supports = true;
            else if (std::strcmp(argv[i], "--tree") == 0) {
                s.supports = true;
                s.tree_supports = true;
            }
            else if (std::strcmp(argv[i], "--ironing") == 0) s.ironing = true;
            else if (std::strcmp(argv[i], "--vase") == 0) s.vase = true;
            else if (std::strcmp(argv[i], "--adaptive") == 0) s.adaptive = true;
            else if (std::strcmp(argv[i], "--raft") == 0 && i + 1 < argc) s.raft = std::atoi(argv[++i]);
            else if (std::strcmp(argv[i], "--brim") == 0 && i + 1 < argc) s.brim = std::atoi(argv[++i]);
            else if (std::strcmp(argv[i], "--printer") == 0 && i + 1 < argc) {
                int idx = ax::find_printer(argv[++i]);
                if (idx >= 0) ax::apply_printer(s, idx);
            } else if (std::strcmp(argv[i], "--filament") == 0 && i + 1 < argc) {
                int idx = ax::find_filament(argv[++i]);
                if (idx >= 0) ax::apply_filament(s, idx);
            } else if (std::strcmp(argv[i], "--flavor") == 0 && i + 1 < argc) {
                const char* f = argv[++i];
                if (std::strcmp(f, "klipper") == 0) s.flavor = ax::GcodeFlavor::Klipper;
                else if (std::strcmp(f, "marlin") == 0) s.flavor = ax::GcodeFlavor::Marlin;
                else s.flavor = ax::GcodeFlavor::Prusa;
            }
        }
        auto r = ax::slice_mesh(d.body, s);
        if (!r.ok) {
            std::fprintf(stderr, "%s\n", r.error.c_str());
            return 1;
        }
        if (!ax::write_gcode(argv[3], r, &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  layers=%zu  filament=%.2f g  time=%.0f s  slice=%.1f ms\n", argv[3], r.layers.size(),
                    r.filament_g, r.time_s, r.slice_ms);
        return 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--drawing") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        if (!d.export_drawing(argv[3], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  holes=%zu  revs=%zu\n", argv[3], d.hole_table().size(), d.rev_log.size());
        return 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--mill") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        for (int i = 4; i < argc; ++i) {
            if (std::strcmp(argv[i], "--tool") == 0 && i + 1 < argc) d.mill.tool_d = (float)std::atof(argv[++i]);
            else if (std::strcmp(argv[i], "--stepover") == 0 && i + 1 < argc)
                d.mill.stepover = (float)std::atof(argv[++i]);
            else if (std::strcmp(argv[i], "--stepdown") == 0 && i + 1 < argc)
                d.mill.stepdown = (float)std::atof(argv[++i]);
            else if (std::strcmp(argv[i], "--face") == 0) {
                const ax::Mesh& m = d.part.empty() ? d.body : d.part;
                ax::FaceRef fr = ax::pick_largest_face(m, {0, 0, 1});
                if (fr.ok) {
                    d.mill.use_face = true;
                    d.mill.face = fr;
                }
            }
        }
        if (!d.export_mill(argv[3], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  holes=%zu  tool=Ø%.1f  face=%s\n", argv[3], d.hole_table().size(), d.mill.tool_d,
                    d.mill.use_face ? "yes" : "no");
        return 0;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--topo") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        ax::Mesh& m = d.body;
        if (!m.topo_ok) m.build_topology();
        int creases = 0;
        for (auto c : m.he_crease)
            if (c) ++creases;
        std::printf("tris=%zu  weld=%zu  halfedges=%zu  crease_he=%d  overlay=%zu\n", m.indices.size() / 3,
                    m.wpos.size(), m.halfs.size(), creases, m.edges.size() / 2);
        if (m.edges.size() >= 2) {
            ax::Vec3 a = m.edges[0], b = m.edges[1];
            ax::Vec3 n1{0, 0, 1};
            bool closed = false;
            auto chain = m.crease_chain(a, b, n1, 48, &closed);
            std::printf("chain0  segs=%zu  loop=%s  a=(%.2f,%.2f,%.2f)  b=(%.2f,%.2f,%.2f)\n", chain.size(),
                        closed ? "yes" : "no", a.x, a.y, a.z, b.x, b.y, b.z);
            for (size_t i = 0; i < chain.size(); ++i)
                std::printf("  %zu  %.3f %.3f %.3f\n", i, chain[i].x, chain[i].y, chain[i].z);
            ax::FaceRef fr = ax::pick_largest_face(m, {0, 0, 1});
            if (fr.ok) {
                auto loops = m.face_loops(fr.p, fr.n);
                std::printf("face_loops  n=%zu  n=(%.2f,%.2f,%.2f)  key=%llu\n", loops.size(), fr.n.x, fr.n.y, fr.n.z,
                            (unsigned long long)fr.key);
                for (size_t i = 0; i < loops.size(); ++i)
                    std::printf("  loop %zu  verts=%zu\n", i, loops[i].size());
            }
            ax::CreaseId cid = m.crease_id(a, b);
            if (cid.ok)
                std::printf("crease_id  fa=%llu fb=%llu slot=%d\n", (unsigned long long)cid.fa,
                            (unsigned long long)cid.fb, cid.slot);
        }
        for (const auto& f : d.features) {
            if (!f.edge.ok) continue;
            std::printf("feature %d  EDGEID fa=%llu fb=%llu slot=%d  loop=%d count=%d  chain=%zu\n", f.id,
                        (unsigned long long)f.edge.fa, (unsigned long long)f.edge.fb, f.edge.slot, f.edge.loop ? 1 : 0,
                        f.count, f.chain.size());
        }
        return 0;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--rebind") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        ax::Feature* box = nullptr;
        ax::Feature* fillet = nullptr;
        for (auto& f : d.features) {
            if (!box && f.kind == ax::FeatureKind::Box) box = &f;
            if (!fillet && (f.kind == ax::FeatureKind::Fillet || f.kind == ax::FeatureKind::Chamfer) && f.style == 1)
                fillet = &f;
        }
        if (!box || !fillet) {
            std::fprintf(stderr, "need a box and a local fillet\n");
            return 2;
        }
        float old_h = box->size.z;
        float z0 = fillet->chain.empty() ? fillet->origin.z : fillet->chain.front().z;
        std::printf("before  box.z=%.2f  chain=%zu  z0=%.2f  edge=%d\n", old_h, fillet->chain.size(), z0,
                    fillet->edge.ok ? 1 : 0);
        float new_h = old_h + 20.f;
        box->size.z = new_h;
        d.rebuild(true);
        float z1 = fillet->chain.empty() ? fillet->origin.z : fillet->chain.front().z;
        float zmax = z1, zmin = z1;
        for (const auto& p : fillet->chain) {
            zmin = std::min(zmin, p.z);
            zmax = std::max(zmax, p.z);
        }
        std::printf("after   box.z=%.2f  chain=%zu  z=%.2f..%.2f  edge=%d  tris=%zu\n", box->size.z,
                    fillet->chain.size(), zmin, zmax, fillet->edge.ok ? 1 : 0, d.body.indices.size() / 3);
        bool top = z0 > old_h - 4.f;
        bool ok = fillet->edge.ok && fillet->chain.size() >= 2 && d.body.indices.size() > 100;
        if (top) ok = ok && zmax > new_h - 2.f && zmin > new_h - 4.f;
        else ok = ok && zmax > old_h + 8.f;
        std::printf("rebind-z  %s\n", ok ? "ok" : "FAIL");
        ax::Document w;
        if (!w.open_any(argv[2], &err)) {
            std::printf("rebind  %s\n", ok ? "ok" : "FAIL");
            return ok ? 0 : 3;
        }
        ax::Feature *wbox = nullptr, *wfil = nullptr;
        for (auto& f : w.features) {
            if (!wbox && f.kind == ax::FeatureKind::Box) wbox = &f;
            if (!wfil && (f.kind == ax::FeatureKind::Fillet || f.kind == ax::FeatureKind::Chamfer) && f.style == 1)
                wfil = &f;
        }
        bool wok = ok;
        if (wbox && wfil && wfil->edge.ok) {
            float old_x = wbox->size.x;
            wbox->size.x = old_x + 20.f;
            w.rebuild(true);
            float xmin = 1e9f, xmax = -1e9f;
            for (const auto& p : wfil->chain) {
                xmin = std::min(xmin, p.x);
                xmax = std::max(xmax, p.x);
            }
            std::printf("after-x  box.x=%.2f  chain=%zu  x=%.2f..%.2f  tris=%zu\n", wbox->size.x, wfil->chain.size(),
                        xmin, xmax, w.body.indices.size() / 3);
            wok = wok && wfil->chain.size() >= 2 && xmax > old_x + 15.f;
            std::printf("rebind-x  %s\n", wok ? "ok" : "FAIL");
        }
        std::printf("rebind  %s\n", wok ? "ok" : "FAIL");
        return wok ? 0 : 3;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--inspect") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        if (d.gdt.empty()) d.suggest_gdt();
        auto rows = d.inspect();
        int fail = 0;
        for (const auto& r : rows) {
            if (!r.pass) ++fail;
            std::printf("%s  %s  nom=%.3f  %+0.3f/−%.3f  act=%.3f  %s  %s\n", r.item.c_str(), r.type.c_str(), r.nom,
                        r.plus, r.minus, r.actual, r.pass ? "PASS" : "FAIL", r.note.c_str());
        }
        const char* out = argc >= 4 ? argv[3] : "/tmp/axiom-inspect.csv";
        if (!d.export_inspect(out, &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  checks=%zu  fail=%d  gdt=%zu  datums=%zu\n", out, rows.size(), fail, d.gdt.size(),
                    d.datums.size());
        return 0;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--dfm") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        auto iss = d.dfm();
        for (const auto& i : iss) std::printf("%s  %s\n", i.sev, i.msg.c_str());
        const char* out = argc >= 4 ? argv[3] : "/tmp/axiom-dfm.txt";
        if (!d.export_dfm(out, &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  issues=%zu\n", out, iss.size());
        return 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--turn") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        if (!d.export_turn(argv[3], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  turn axis=Z  bounds z=%.2f..%.2f\n", argv[3],
                    d.body.bounds.valid() ? d.body.bounds.mn.z : 0.f,
                    d.body.bounds.valid() ? d.body.bounds.mx.z : 0.f);
        return 0;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--fits") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        for (const auto& h : d.hole_table()) {
            std::printf("%s  Ø%.3f  %s  %.3f…%.3f  %s  (%.1f, %.1f, %.1f)\n", h.name.c_str(), h.dia, h.fit.c_str(),
                        h.fit_min, h.fit_max, h.type, h.origin.x, h.origin.y, h.origin.z);
        }
        return 0;
    }
    if (argc >= 3 && std::strcmp(argv[1], "--stack") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        auto st = d.stack();
        for (const auto& s : st.segs)
            std::printf("  %s  nom=%.3f  %+0.3f/−%.3f\n", s.name.c_str(), s.nom, s.plus, s.minus);
        std::printf("stack  nom=%.3f  WC=%.3f…%.3f  RSS=±%.3f  segs=%zu\n", st.nom, st.wc_min, st.wc_max, st.rss,
                    st.segs.size());
        return 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--export") == 0) {
        std::string err;
        ax::Document d;
        if (!d.open_any(argv[2], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        if (!d.export_any(argv[3], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  triangles=%zu\n", argv[3], d.body.indices.size() / 3);
        return 0;
    }
    if (argc >= 2 && std::strcmp(argv[1], "--export-demo-stl") == 0) {
        ax::Document d = ax::Document::demo_bracket();
        std::string err;
        const char* out = argc >= 3 ? argv[2] : "/tmp/axiom-bracket.stl";
        if (!d.export_stl(out, &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  triangles=%zu  rebuild=%.2f ms\n", out, d.body.indices.size() / 3,
                    d.last_rebuild_ms);
        return d.body.empty() ? 2 : 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--export-svg") == 0) {
        std::string err;
        auto d = ax::Document::load(argv[2], &err);
        if (!d) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        if (!d->export_svg(argv[3], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s\n", argv[3]);
        return 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--export-dxf") == 0) {
        std::string err;
        auto d = ax::Document::load(argv[2], &err);
        if (!d) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        if (!d->export_dxf(argv[3], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s\n", argv[3]);
        return 0;
    }
    if (argc >= 2 && std::strcmp(argv[1], "--bench") == 0) {
        ax::Document d = ax::Document::demo_housing();
        if (argc >= 3 && std::strcmp(argv[2], "gear") == 0) d = ax::Document::demo_gear();
        else if (argc >= 3 && std::strcmp(argv[2], "ribbed") == 0) d = ax::Document::demo_ribbed();
        else if (argc >= 3 && std::strcmp(argv[2], "bracket") == 0) d = ax::Document::demo_bracket();
        else if (argc >= 3 && std::strcmp(argv[2], "coupling") == 0) d = ax::Document::demo_coupling();
        else if (argc >= 3 && std::strcmp(argv[2], "pillow") == 0) d = ax::Document::demo_pillow();
        else if (argc >= 3 && std::strcmp(argv[2], "vblock") == 0) d = ax::Document::demo_vblock();
        else if (argc >= 3 && std::strcmp(argv[2], "hanger") == 0) d = ax::Document::demo_hanger();
        double first = d.last_rebuild_ms;
        d.rebuild(false);
        double cached = d.last_rebuild_ms;
        d.rebuild(true);
        double forced = d.last_rebuild_ms;
        std::printf("triangles=%zu  first=%.2f ms  cached=%.2f ms  forced=%.2f ms\n", d.body.indices.size() / 3,
                    first, cached, forced);
        return d.body.empty() ? 2 : 0;
    }
    if (argc >= 4 && std::strcmp(argv[1], "--export-stl") == 0) {
        std::string err;
        auto d = ax::Document::load(argv[2], &err);
        if (!d) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        if (!d->export_stl(argv[3], &err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        std::printf("wrote %s  triangles=%zu\n", argv[3], d->body.indices.size() / 3);
        return 0;
    }
    if (argc >= 2 && std::strcmp(argv[1], "--analyze") == 0) {
        ax::Document d = ax::Document::demo_housing();
        if (argc >= 3 && std::strcmp(argv[2], "bracket") == 0) d = ax::Document::demo_bracket();
        else if (argc >= 3 && std::strcmp(argv[2], "flange") == 0) d = ax::Document::demo_flange();
        else if (argc >= 3 && std::strcmp(argv[2], "pulley") == 0) d = ax::Document::demo_pulley();
        else if (argc >= 3 && std::strcmp(argv[2], "coupling") == 0) d = ax::Document::demo_coupling();
        else if (argc >= 3 && std::strcmp(argv[2], "pillow") == 0) d = ax::Document::demo_pillow();
        else if (argc >= 3 && std::strcmp(argv[2], "vblock") == 0) d = ax::Document::demo_vblock();
        else if (argc >= 3 && std::strcmp(argv[2], "hanger") == 0) d = ax::Document::demo_hanger();
        for (int i = 2; i < argc; ++i) {
            if (std::strcmp(argv[i], "--modal") == 0) d.analysis.mode = ax::AnalysisMode::Modal;
            else if (std::strcmp(argv[i], "--thermal") == 0) d.analysis.mode = ax::AnalysisMode::Thermal;
            else if (std::strcmp(argv[i], "--static") == 0) d.analysis.mode = ax::AnalysisMode::Static;
            else if (std::strcmp(argv[i], "--case2") == 0) d.analysis_slot = 1;
            else if (std::strcmp(argv[i], "--compare") == 0) d.analysis_slot = 2;
        }
        auto r = d.run_analysis();
        const ax::Material& mat = ax::material_at(d.material_id);
        std::printf("material: %s  E=%.1f GPa  σy=%.0f MPa  mode=%s\n", mat.name, mat.E, mat.yield,
                    ax::analysis_mode_name(d.analysis.mode));
        std::printf("mass: %.3f g  volume: %.1f mm³  cost: $%.4f\n", d.mass.mass_g, d.mass.volume_mm3,
                    d.mass.cost_usd);
        if (!r.ok) {
            std::fprintf(stderr, "analysis failed\n");
            return 2;
        }
        if (d.analysis.mode == ax::AnalysisMode::Thermal)
            std::printf("thermal: Tmax %.3f °C  ΔT %.3f  SF(Tmax) %.2f  %.1f ms\n", r.max_vm, r.max_u, r.safety,
                        r.ms);
        else if (d.analysis.mode == ax::AnalysisMode::Modal)
            std::printf("modal: f1 ≈ %.3f Hz  %.1f ms\n", r.max_u, r.ms);
        else
            std::printf("load: %.0f N  vonMises max: %.3f MPa  mean: %.3f MPa  |u|max: %.4f mm  SF: %.2f  %.1f ms\n",
                        r.load_n, r.max_vm, r.mean_vm, r.max_u, r.safety, r.ms);
        return 0;
    }
    if (argc >= 2 && std::strcmp(argv[1], "--demo") == 0) {
        ax::Document d = ax::Document::demo_bracket();
        if (argc >= 3 && std::strcmp(argv[2], "pulley") == 0) d = ax::Document::demo_pulley();
        else if (argc >= 3 && std::strcmp(argv[2], "housing") == 0) d = ax::Document::demo_housing();
        else if (argc >= 3 && std::strcmp(argv[2], "flange") == 0) d = ax::Document::demo_flange();
        else if (argc >= 3 && std::strcmp(argv[2], "assembly") == 0) d = ax::Document::demo_assembly();
        else if (argc >= 3 && std::strcmp(argv[2], "handle") == 0) d = ax::Document::demo_handle();
        else if (argc >= 3 && std::strcmp(argv[2], "tray") == 0) d = ax::Document::demo_tray();
        else if (argc >= 3 && std::strcmp(argv[2], "spring") == 0) d = ax::Document::demo_spring();
        else if (argc >= 3 && std::strcmp(argv[2], "gear") == 0) d = ax::Document::demo_gear();
        else if (argc >= 3 && std::strcmp(argv[2], "ribbed") == 0) d = ax::Document::demo_ribbed();
        else if (argc >= 3 && std::strcmp(argv[2], "elbow") == 0) d = ax::Document::demo_elbow();
        else if (argc >= 3 && std::strcmp(argv[2], "coupling") == 0) d = ax::Document::demo_coupling();
        else if (argc >= 3 && std::strcmp(argv[2], "pillow") == 0) d = ax::Document::demo_pillow();
        else if (argc >= 3 && std::strcmp(argv[2], "vblock") == 0) d = ax::Document::demo_vblock();
        else if (argc >= 3 && std::strcmp(argv[2], "hanger") == 0) d = ax::Document::demo_hanger();
        std::printf("%s", d.serialize().c_str());
        std::fprintf(stderr, "triangles=%zu rebuild=%.2f ms components=%zu mass=%.2f g\n",
                     d.body.indices.size() / 3, d.last_rebuild_ms, d.components.size(), d.mass.mass_g);
        return d.body.empty() ? 2 : 0;
    }

    ax::App app;
    if (!app.init()) {
        std::fprintf(stderr, "Axiom failed to start: %s\n", app.error.empty() ? "unknown" : app.error.c_str());
        return 1;
    }
    if (argc >= 2 && argv[1][0] != '-') {
        std::string err;
        ax::Document d;
        if (d.open_any(argv[1], &err)) {
            app.doc = std::move(d);
            app.renderer.uploaded_rev = ~std::uint64_t{0};
            app.renderer.implicit = !app.doc.has_imports();
            app.camera.frame(app.doc.body.bounds);
            app.status = "Opened part.";
        } else {
            app.status = err;
        }
    }
    app.run();
    app.shutdown();
    return 0;
}
