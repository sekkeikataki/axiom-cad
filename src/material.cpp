#include "material.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

namespace ax {

static const Material kMats[] = {
    {"6061-T6 aluminium", "Aluminium", "UNS A96061", 2.70f, 68.9f, 0.33f, 276.f, 310.f, 23.6f, 167.f, 150.f, 2.80f, {0.82f, 0.84f, 0.86f}},
    {"7075-T6 aluminium", "Aluminium", "UNS A97075", 2.81f, 71.7f, 0.33f, 503.f, 572.f, 23.4f, 130.f, 125.f, 4.50f, {0.78f, 0.80f, 0.83f}},
    {"304 stainless", "Steel", "UNS S30400", 8.00f, 193.f, 0.29f, 215.f, 505.f, 17.3f, 16.2f, 870.f, 3.20f, {0.72f, 0.74f, 0.76f}},
    {"316L stainless", "Steel", "UNS S31603", 8.00f, 193.f, 0.30f, 170.f, 485.f, 16.0f, 13.4f, 870.f, 4.10f, {0.70f, 0.73f, 0.76f}},
    {"4140 alloy steel", "Steel", "UNS G41400", 7.85f, 205.f, 0.29f, 415.f, 655.f, 12.3f, 42.6f, 425.f, 1.90f, {0.45f, 0.47f, 0.50f}},
    {"Ti-6Al-4V", "Titanium", "Grade 5", 4.43f, 113.8f, 0.34f, 880.f, 950.f, 8.6f, 6.7f, 400.f, 28.0f, {0.62f, 0.64f, 0.70f}},
    {"C360 brass", "Copper alloy", "UNS C36000", 8.50f, 97.f, 0.31f, 124.f, 338.f, 20.5f, 115.f, 200.f, 6.50f, {0.82f, 0.68f, 0.28f}},
    {"C110 copper", "Copper", "UNS C11000", 8.94f, 117.f, 0.33f, 69.f, 220.f, 17.0f, 388.f, 200.f, 9.20f, {0.80f, 0.48f, 0.22f}},
    {"Cast iron A48-40", "Iron", "ASTM A48", 7.15f, 100.f, 0.26f, 276.f, 276.f, 10.5f, 46.f, 350.f, 1.10f, {0.35f, 0.35f, 0.36f}},
    {"Inconel 718", "Nickel", "UNS N07718", 8.19f, 200.f, 0.29f, 1034.f, 1237.f, 13.0f, 11.4f, 700.f, 42.0f, {0.58f, 0.60f, 0.64f}},
    {"ABS-GF", "Polymer", "Extruded", 1.04f, 2.3f, 0.35f, 40.f, 40.f, 90.f, 0.18f, 80.f, 2.40f, {0.20f, 0.22f, 0.26f}},
    {"Nylon 6/6", "Polymer", "PA66", 1.14f, 2.9f, 0.39f, 70.f, 80.f, 80.f, 0.25f, 90.f, 3.80f, {0.22f, 0.24f, 0.30f}},
    {"POM (Delrin)", "Polymer", "Acetal", 1.41f, 3.1f, 0.35f, 66.f, 70.f, 110.f, 0.31f, 90.f, 4.20f, {0.85f, 0.86f, 0.88f}},
    {"PEEK", "Polymer", "Unfilled", 1.30f, 3.6f, 0.38f, 90.f, 100.f, 47.f, 0.25f, 250.f, 85.f, {0.55f, 0.48f, 0.40f}},
    {"Alumina 96%", "Ceramic", "Al2O3", 3.70f, 300.f, 0.21f, 200.f, 200.f, 8.0f, 24.f, 1400.f, 12.f, {0.90f, 0.90f, 0.92f}},
};

const Material* materials(int& n) {
    n = (int)(sizeof(kMats) / sizeof(kMats[0]));
    return kMats;
}
int material_count() { return (int)(sizeof(kMats) / sizeof(kMats[0])); }
const Material& material_at(int id) {
    int n = material_count();
    if (id < 0 || id >= n) return kMats[0];
    return kMats[id];
}

MassProps mass_properties(const Mesh& mesh, const Material& mat) {
    MassProps r;
    if (mesh.indices.size() < 3) return r;
    double vol = 0, cx = 0, cy = 0, cz = 0, area = 0;
    double Ixx = 0, Iyy = 0, Izz = 0;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 a = mesh.vertices[mesh.indices[i]].p;
        Vec3 b = mesh.vertices[mesh.indices[i + 1]].p;
        Vec3 c = mesh.vertices[mesh.indices[i + 2]].p;
        double tet = (double)a.x * (b.y * c.z - b.z * c.y) + (double)a.y * (b.z * c.x - b.x * c.z) +
                     (double)a.z * (b.x * c.y - b.y * c.x);
        tet /= 6.0;
        vol += tet;
        Vec3 g = (a + b + c) * 0.25f; // tet centroid with origin as 4th vertex, approx
        cx += tet * g.x;
        cy += tet * g.y;
        cz += tet * g.z;
        Vec3 e1 = b - a, e2 = c - a;
        area += 0.5 * e1.cross(e2).length();
        // tetra inertia about origin (scaled); refined after COM shift
        Ixx += tet * (g.y * g.y + g.z * g.z);
        Iyy += tet * (g.x * g.x + g.z * g.z);
        Izz += tet * (g.x * g.x + g.y * g.y);
    }
    vol = std::fabs(vol);
    r.volume_mm3 = vol;
    r.area_mm2 = area;
    if (vol > 1e-8) {
        r.com = {(float)(cx / (vol == 0 ? 1 : (cx + cy + cz != 0 && std::fabs(vol) > 0 ? vol : 1))),
                 (float)(cy / vol), (float)(cz / vol)};
        if (std::fabs(vol) > 1e-12) r.com = {(float)(cx / vol), (float)(cy / vol), (float)(cz / vol)};
    }
    // density g/cm³ → g/mm³ = density / 1000
    double rho = mat.density / 1000.0;
    r.mass_g = vol * rho;
    // Parallel-axis to COM (diagonal only)
    double mx = r.com.x, my = r.com.y, mz = r.com.z;
    r.I = {(float)(std::fabs(Ixx - vol * (my * my + mz * mz)) * rho),
           (float)(std::fabs(Iyy - vol * (mx * mx + mz * mz)) * rho),
           (float)(std::fabs(Izz - vol * (mx * mx + my * my)) * rho)};
    r.cost_usd = (r.mass_g / 1000.0) * mat.cost;
    return r;
}

StressResult analyze_stress(const SdfScene& scene, Mesh& paint, const Material& mat, float load_n, int grid) {
    StressResult out;
    auto t0 = std::chrono::high_resolution_clock::now();
    Aabb bb = bounds_scene(scene, -1);
    if (!bb.valid() || paint.empty()) return out;
    bb = bb.padded(0.5f);
    int N = std::max(12, std::min(grid, 36));
    Vec3 span = bb.size();
    float cell = std::max(span.max_comp() / (float)N, 0.4f);
    int nx = std::max(6, std::min(N, (int)std::ceil(span.x / cell)));
    int ny = std::max(6, std::min(N, (int)std::ceil(span.y / cell)));
    int nz = std::max(6, std::min(N, (int)std::ceil(span.z / cell)));
    const int nv = nx * ny * nz;
    std::vector<char> occ(nv, 0);
    auto vid = [&](int x, int y, int z) { return (z * ny + y) * nx + x; };
    auto pos = [&](int x, int y, int z) {
        return bb.mn + Vec3{(x + 0.5f) * cell, (y + 0.5f) * cell, (z + 0.5f) * cell};
    };

    int nocc = 0;
    {
        int nt = std::max(1, (int)std::thread::hardware_concurrency());
        nt = std::min(nt, nz);
        std::vector<int> local(nt, 0);
        std::vector<std::thread> pool;
        int chunk = (nz + nt - 1) / nt;
        for (int t = 0; t < nt; ++t) {
            int z0 = t * chunk, z1 = std::min(nz, z0 + chunk);
            if (z0 >= z1) break;
            pool.emplace_back([&, t, z0, z1] {
                int n = 0;
                for (int z = z0; z < z1; ++z)
                    for (int y = 0; y < ny; ++y)
                        for (int x = 0; x < nx; ++x)
                            if (eval_scene(scene, pos(x, y, z), -1) < 0) {
                                occ[vid(x, y, z)] = 1;
                                ++n;
                            }
                local[t] = n;
            });
        }
        for (auto& th : pool) th.join();
        for (int n : local) nocc += n;
    }
    if (nocc < 8) return out;

    std::vector<float> ux(nv, 0), uy(nv, 0), uz(nv, 0);
    std::vector<char> fixed(nv, 0), loaded(nv, 0);
    int nfix = 0, nload = 0;
    int zmin_occ = nz, zmax_occ = -1;
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x)
                if (occ[vid(x, y, z)]) {
                    zmin_occ = std::min(zmin_occ, z);
                    zmax_occ = std::max(zmax_occ, z);
                }
    if (zmax_occ < zmin_occ) return out;
    int zfix_i = zmin_occ + std::max(1, (zmax_occ - zmin_occ) / 10);
    int zload_i = zmax_occ - std::max(1, (zmax_occ - zmin_occ) / 10);
    if (zload_i <= zfix_i) {
        zfix_i = zmin_occ;
        zload_i = zmax_occ;
    }
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x) {
                int i = vid(x, y, z);
                if (!occ[i]) continue;
                if (z <= zfix_i) {
                    fixed[i] = 1;
                    ++nfix;
                } else if (z >= zload_i) {
                    loaded[i] = 1;
                    ++nload;
                }
            }
    if (nload < 1) {
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x)
                if (occ[vid(x, y, zmax_occ)]) {
                    loaded[vid(x, y, zmax_occ)] = 1;
                    ++nload;
                }
    }
    nload = std::max(1, nload);
    float xmin = 1e9f, xmax = -1e9f, ymin = 1e9f, ymax = -1e9f;
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x) {
                int i = vid(x, y, z);
                if (!occ[i] || !loaded[i]) continue;
                Vec3 p = pos(x, y, z);
                xmin = std::min(xmin, p.x);
                xmax = std::max(xmax, p.x);
                ymin = std::min(ymin, p.y);
                ymax = std::max(ymax, p.y);
            }
    float area = std::max((xmax - xmin + cell) * (ymax - ymin + cell), nload * cell * cell);
    area = std::max(area, cell * cell);
    float length = std::max((zload_i - zfix_i) * cell, cell);
    float E_mpa = std::max(mat.E * 1000.f, 100.f);
    float u_app = (std::fabs(load_n) * length) / (area * E_mpa);
    u_app = std::max(u_app, 1e-6f);

    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x) {
                int i = vid(x, y, z);
                if (!occ[i] || fixed[i]) continue;
                float t = (float)(z - zfix_i) / (float)std::max(1, zload_i - zfix_i);
                t = clamp(t, 0.f, 1.f);
                uz[i] = -u_app * t;
                if (loaded[i]) uz[i] = -u_app;
            }

    const int iters = 80;
    const float w = 1.6f; // SOR
    for (int it = 0; it < iters; ++it) {
        for (int z = 0; z < nz; ++z)
            for (int y = 0; y < ny; ++y)
                for (int x = 0; x < nx; ++x) {
                    int i = vid(x, y, z);
                    if (!occ[i] || fixed[i] || loaded[i]) continue;
                    float sx = 0, sy = 0, sz = 0;
                    int c = 0;
                    auto acc = [&](int x2, int y2, int z2) {
                        if (x2 < 0 || y2 < 0 || z2 < 0 || x2 >= nx || y2 >= ny || z2 >= nz) return;
                        int j = vid(x2, y2, z2);
                        if (!occ[j]) return;
                        sx += ux[j];
                        sy += uy[j];
                        sz += uz[j];
                        ++c;
                    };
                    acc(x - 1, y, z);
                    acc(x + 1, y, z);
                    acc(x, y - 1, z);
                    acc(x, y + 1, z);
                    acc(x, y, z - 1);
                    acc(x, y, z + 1);
                    if (c == 0) continue;
                    ux[i] = lerp(ux[i], sx / c, w);
                    uy[i] = lerp(uy[i], sy / c, w);
                    uz[i] = lerp(uz[i], sz / c, w);
                }
    }

    std::vector<float> vm(nv, 0);
    double sum = 0;
    int ns = 0;
    float maxvm = 0, maxu = 0;
    auto u_at = [&](int x, int y, int z, int c) {
        x = std::max(0, std::min(nx - 1, x));
        y = std::max(0, std::min(ny - 1, y));
        z = std::max(0, std::min(nz - 1, z));
        int i = vid(x, y, z);
        if (!occ[i]) i = vid(std::max(0, std::min(nx - 1, x)), std::max(0, std::min(ny - 1, y)),
                             std::max(0, std::min(nz - 1, z)));
        return c == 0 ? ux[i] : c == 1 ? uy[i] : uz[i];
    };
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x) {
                int i = vid(x, y, z);
                if (!occ[i]) continue;
                float exx = (u_at(x + 1, y, z, 0) - u_at(x - 1, y, z, 0)) / (2 * cell);
                float eyy = (u_at(x, y + 1, z, 1) - u_at(x, y - 1, z, 1)) / (2 * cell);
                float ezz = (u_at(x, y, z + 1, 2) - u_at(x, y, z - 1, 2)) / (2 * cell);
                float gxy = (u_at(x + 1, y, z, 1) - u_at(x - 1, y, z, 1) + u_at(x, y + 1, z, 0) -
                             u_at(x, y - 1, z, 0)) /
                            (4 * cell);
                float gxz = (u_at(x + 1, y, z, 2) - u_at(x - 1, y, z, 2) + u_at(x, y, z + 1, 0) -
                             u_at(x, y, z - 1, 0)) /
                            (4 * cell);
                float gyz = (u_at(x, y + 1, z, 2) - u_at(x, y - 1, z, 2) + u_at(x, y, z + 1, 1) -
                             u_at(x, y, z - 1, 1)) /
                            (4 * cell);
                float E = mat.E * 1000.f;
                float nu = clamp(mat.nu, 0.05f, 0.49f);
                float lam = E * nu / ((1 + nu) * (1 - 2 * nu));
                float mu = E / (2 * (1 + nu));
                float tr = exx + eyy + ezz;
                float sxx = lam * tr + 2 * mu * exx;
                float syy = lam * tr + 2 * mu * eyy;
                float szz = lam * tr + 2 * mu * ezz;
                float sxy = mu * gxy, sxz = mu * gxz, syz = mu * gyz;
                float se = std::sqrt(0.5f * ((sxx - syy) * (sxx - syy) + (syy - szz) * (syy - szz) +
                                             (szz - sxx) * (szz - sxx)) +
                                     3.f * (sxy * sxy + sxz * sxz + syz * syz));
                vm[i] = se;
                float u = std::sqrt(ux[i] * ux[i] + uy[i] * uy[i] + uz[i] * uz[i]);
                maxu = std::max(maxu, u);
                if (!fixed[i] && !loaded[i]) {
                    maxvm = std::max(maxvm, se);
                    sum += se;
                    ++ns;
                }
            }
    if (maxvm < 1e-6f) {
        for (int i = 0; i < nv; ++i)
            if (occ[i]) maxvm = std::max(maxvm, vm[i]);
    }

    auto sample_vm = [&](Vec3 p) {
        float fx = (p.x - bb.mn.x) / cell - 0.5f;
        float fy = (p.y - bb.mn.y) / cell - 0.5f;
        float fz = (p.z - bb.mn.z) / cell - 0.5f;
        int x = (int)std::floor(fx), y = (int)std::floor(fy), z = (int)std::floor(fz);
        x = std::max(0, std::min(nx - 2, x));
        y = std::max(0, std::min(ny - 2, y));
        z = std::max(0, std::min(nz - 2, z));
        float tx = clamp(fx - x, 0.f, 1.f), ty = clamp(fy - y, 0.f, 1.f), tz = clamp(fz - z, 0.f, 1.f);
        auto at = [&](int xi, int yi, int zi) { return vm[vid(xi, yi, zi)]; };
        float c00 = lerp(at(x, y, z), at(x + 1, y, z), tx);
        float c10 = lerp(at(x, y + 1, z), at(x + 1, y + 1, z), tx);
        float c01 = lerp(at(x, y, z + 1), at(x + 1, y, z + 1), tx);
        float c11 = lerp(at(x, y + 1, z + 1), at(x + 1, y + 1, z + 1), tx);
        return lerp(lerp(c00, c10, ty), lerp(c01, c11, ty), tz);
    };
    float denom = std::max(maxvm, 1e-4f);
    for (auto& v : paint.vertices) v.scalar = sample_vm(v.p) / denom;

    out.ok = true;
    out.max_vm = maxvm;
    out.mean_vm = ns ? (float)(sum / ns) : 0;
    out.max_u = maxu;
    out.safety = maxvm > 1e-4f ? mat.yield / maxvm : 99.f;
    out.load_n = load_n;
    out.iters = iters;
    out.ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t0).count();
    return out;
}

} // namespace ax
