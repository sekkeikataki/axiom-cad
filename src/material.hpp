#pragma once

#include "mesh.hpp"
#include <string>
#include <vector>

namespace ax {

struct Material {
    const char* name;
    const char* family;
    const char* spec;     // standard / temper
    float density;        // g/cm³
    float E;              // GPa, Young's modulus
    float nu;             // Poisson
    float yield;          // MPa, 0.2% proof
    float uts;            // MPa, ultimate
    float cte;            // 10⁻⁶ /K
    float k_therm;        // W/m·K
    float Tmax;           // °C service
    float cost;           // USD/kg
    Vec3 color;
};

const Material* materials(int& n);
const Material& material_at(int id);
int material_count();

struct MassProps {
    double volume_mm3 = 0;
    double area_mm2 = 0;
    double mass_g = 0;
    Vec3 com;
    Vec3 I; // g·mm² diagonal about COM (Ixx, Iyy, Izz)
    double cost_usd = 0;
};

MassProps mass_properties(const Mesh& mesh, const Material& mat);

struct StressResult {
    bool ok = false;
    float max_vm = 0;     // MPa
    float mean_vm = 0;
    float max_u = 0;      // mm displacement
    float safety = 0;     // yield / max_vm
    float load_n = 0;
    int iters = 0;
    double ms = 0;
};

// Linear-elastic voxel lattice. Fixes the low-Z face, loads the high-Z face in -Z.
StressResult analyze_stress(const SdfScene& scene, Mesh& paint, const Material& mat, float load_n, int grid = 26);

} // namespace ax
