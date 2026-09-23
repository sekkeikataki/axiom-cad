#pragma once

#include "document.hpp"
#include <glad/gl.h>
#include <string>
#include <vector>

namespace ax {

enum class Shading : int { ShadedEdges = 0, Shaded = 1, Wire = 2, HiddenLine = 3 };

struct Camera {
    Vec3 target{20, 15, 8};
    float distance = 160;
    float yaw = 35 * kDeg;
    float pitch = 32 * kDeg;
    bool ortho = false;
    float fovy = 42;
    float aspect = 1.6f;
    int w = 1, h = 1;

    Vec3 eye() const;
    Mat4 view() const;
    Mat4 proj() const;
    Ray ray_from_pixel(float x, float y) const; // pixel origin top-left of viewport
    void orbit(float dx, float dy);
    void pan(float dx, float dy);
    void zoom(float factor);
    void frame(const Aabb& b);
    void set_view(const char* name); // front top right iso home
};

struct GpuMesh {
    GLuint vao = 0, vbo = 0, ibo = 0, ebo = 0;
    GLsizei nidx = 0, nedge = 0;
    void destroy();
    void upload(const Mesh& m);
};

struct Renderer {
    GLuint prog_solid = 0, prog_line = 0;
    GLuint fbo = 0, color = 0, depth = 0;
    GLuint msaa_fbo = 0, msaa_color = 0, msaa_depth = 0;
    int msaa_samples = 4;
    int fb_w = 0, fb_h = 0;
    GLuint ss_fbo = 0, ss_color = 0, ss_depth = 0;
    int ss_w = 0, ss_h = 0;
    int ss_scale = 2;
    GpuMesh body, preview;
    std::uint64_t uploaded_rev = ~std::uint64_t{0};
    bool preview_valid = false;
    Shading shading = Shading::ShadedEdges;
    Vec3 body_color{0.78f, 0.82f, 0.86f};
    Vec3 select_color{1.0f, 0.72f, 0.22f};
    std::uint32_t selected_id = 0;
    std::uint32_t hover_id = 0;
    bool show_grid = true;
    bool show_planes = true;
    bool show_origin = true;
    bool section = false;
    Vec3 section_n{0, 0, 1};
    float section_d = 0;
    bool analysis = false;
    bool implicit = true; // ray-march exact SDF + contours (not the export mesh)

    bool init(std::string* err);
    void shutdown();
    void resize(int w, int h);
    GLuint draw(const Camera& cam, const Document& doc, const Mesh* preview_mesh, int active_sketch_plane,
                Vec3 sketch_origin, const std::vector<Vec3>& extra_lines, const std::vector<Vec3>& extra_colors);

    GLuint texture() const { return color; }

private:
    GLuint compile(GLenum type, const char* src, std::string* err);
    GLuint link(GLuint vs, GLuint fs, std::string* err);
    void draw_grid(const Camera& cam);
    void draw_planes();
    void draw_origin();
    void draw_lines(const Camera& cam, const std::vector<Vec3>& pts, const Vec3& color, float width);
    void upload_scene(const Document& doc);
    bool draw_implicit(const Camera& cam, const Document& doc, Vec3 color);

    GLuint prog_trace = 0;
    GLuint vao_fs = 0;
    GLuint prim_bo = 0, prim_tex = 0, curve_bo = 0, curve_tex = 0;
    int nprims = 0;
    std::uint64_t scene_rev = ~std::uint64_t{0};
};

} // namespace ax
