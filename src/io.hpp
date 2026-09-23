#pragma once

#include "mesh.hpp"
#include <string>
#include <vector>

namespace ax {

struct Sketch;
struct Document;

enum class FileKind {
    Axm,
    Stl,
    Obj,
    Ply,
    Off,
    Amf,
    Gltf,
    Glb,
    Wrl,
    X3d,
    Step,
    Iges,
    Dxf,
    Svg,
    Json,
    Xyz,
    ThreeMf,
    Csv,
    Png,
    Ppm,
    Dae,
    Gcode,
    Unknown
};

FileKind file_kind(const std::string& path);
const char* file_kind_name(FileKind k);
const char* file_kind_ext(FileKind k);
bool is_mesh_kind(FileKind k);
bool is_sketch_kind(FileKind k);
bool is_native_kind(FileKind k);
std::string supported_formats_help();
std::string file_stem(const std::string& path);

bool read_mesh_file(const std::string& path, Mesh& out, std::string* err);
bool write_mesh_file(const std::string& path, const Mesh& mesh, std::string* err);
bool read_sketch_file(const std::string& path, Sketch& out, std::string* err);
bool write_dxf_sketch(const std::string& path, const Sketch& sk, std::string* err);
bool write_svg_sketch(const std::string& path, const Sketch& sk, std::string* err);
bool read_xyz_points(const std::string& path, std::vector<Vec3>& out, std::string* err);

bool write_step(const std::string& path, const Mesh& mesh, std::string* err);
bool write_iges(const std::string& path, const Mesh& mesh, std::string* err);
bool write_gltf(const std::string& path, const Mesh& mesh, std::string* err);
bool write_wrl(const std::string& path, const Mesh& mesh, std::string* err);
bool write_x3d(const std::string& path, const Mesh& mesh, std::string* err);
bool write_ply(const std::string& path, const Mesh& mesh, std::string* err);
bool write_off(const std::string& path, const Mesh& mesh, std::string* err);
bool write_amf(const std::string& path, const Mesh& mesh, std::string* err);
bool write_3mf(const std::string& path, const Mesh& mesh, std::string* err);
bool write_mesh_json(const std::string& path, const Mesh& mesh, std::string* err);
bool write_dae(const std::string& path, const Mesh& mesh, std::string* err);

Mesh transform_import(const Mesh& src, Vec3 origin, Vec3 euler_deg, Vec3 scale, std::uint32_t fid);
Mesh clip_against_mesh(const Mesh& a, const Mesh& ref, bool keep_inside);
Mesh punch_sdf_cuts(const Mesh& m, const std::vector<SdfPrim>& cuts);

} // namespace ax
