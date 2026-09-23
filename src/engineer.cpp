#include "engineer.hpp"
#include "document.hpp"
#include "inspect.hpp"
#include "slicer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <ostream>
#include <thread>
#include <unordered_map>

namespace ax {

static void gword(std::ostream& o, char w, float v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%c%.3f", w, (double)v);
    o << b;
}

static void g3(std::ostream& o, float v) {
    char b[24];
    std::snprintf(b, sizeof(b), "%.3f", (double)v);
    o << b;
}

const char* analysis_mode_name(AnalysisMode m) {
    switch (m) {
    case AnalysisMode::Static: return "Static";
    case AnalysisMode::Modal: return "Modal";
    case AnalysisMode::Thermal: return "Thermal";
    }
    return "Static";
}

int estimate_dof(int n_free, int n_insert, int n_flush, int n_angle) {
    int d = n_free * 6 - n_insert * 3 - n_flush * 1 - n_angle * 1;
    return std::max(0, d);
}
