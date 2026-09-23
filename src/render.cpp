#include "render.hpp"
#include "material.hpp"
#include "trace_shader.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace ax {

static const char* kSolidVS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in uint aFid;
layout(location=3) in float aS;
uniform mat4 uVP;
out vec3 vN;
out vec3 vW;
out float vS;
flat out uint vFid;
void main(){
    vN = aNrm;
    vW = aPos;
    vS = aS;
    vFid = aFid;
    gl_Position = uVP * vec4(aPos,1);
}
)";

static const char* kSolidFS = R"(
#version 330 core
in vec3 vN;
in vec3 vW;
in float vS;
flat in uint vFid;
uniform vec3 uEye;
uniform vec3 uColor;
uniform uint uSel;
uniform uint uHover;
uniform int uWire;
uniform int uSection;
uniform int uAnalysis;
uniform vec4 uClip;
out vec4 frag;
vec3 turbo(float t){
    t = clamp(t, 0.0, 1.0);
    vec3 c = vec3(0.15, 0.25, 0.85);
    c = mix(c, vec3(0.10, 0.75, 0.85), smoothstep(0.0, 0.25, t));
    c = mix(c, vec3(0.20, 0.85, 0.30), smoothstep(0.25, 0.50, t));
    c = mix(c, vec3(0.95, 0.85, 0.15), smoothstep(0.50, 0.75, t));
    c = mix(c, vec3(0.92, 0.18, 0.12), smoothstep(0.75, 1.0, t));
    return c;
}
void main(){
    if(uSection == 1 && dot(vW, uClip.xyz) > uClip.w) discard;
    vec3 n = normalize(vN);
    vec3 V = normalize(uEye - vW);
    if(dot(n,V) < 0.0) n = -n;
    vec3 L1 = normalize(vec3(0.55, 0.20, 0.90));
    vec3 L2 = normalize(vec3(-0.65, -0.30, 0.35));
    vec3 L3 = normalize(vec3(0.05, 0.85, 0.25));
    float d1 = max(dot(n,L1),0.0);
    float d2 = max(dot(n,L2),0.0);
    float d3 = max(dot(n,L3),0.0);
    vec3 H = normalize(L1 + V);
    float spec = pow(max(dot(n,H),0.0), 96.0) * 0.34;
    float ndl = 0.20 + 0.56*d1 + 0.18*d2 + 0.12*d3;
    float ao = 0.82 + 0.18 * max(n.z, 0.0);
    float rim = pow(1.0 - max(dot(n,V),0.0), 2.8) * 0.14;
    vec3 base = uColor;
    if(uAnalysis == 1) base = turbo(vS);
    vec3 col = base * ndl * ao + vec3(0.92,0.94,0.97)*spec + vec3(0.80,0.86,0.95)*rim;
    if(vFid == uSel && uSel != 0u) col = mix(col, vec3(1.0, 0.70, 0.18), 0.42);
    else if(vFid == uHover && uHover != 0u) col = mix(col, vec3(0.45, 0.78, 1.0), 0.26);
    if(uWire == 1){
        frag = vec4(col * 0.15, 1);
        return;
    }
    frag = vec4(col, 1);
}
)";

static const char* kLineVS = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uVP;
out vec3 vW;
void main(){ vW = aPos; gl_Position = uVP * vec4(aPos,1); }
)";

static const char* kLineFS = R"(
#version 330 core
in vec3 vW;
uniform vec3 uColor;
uniform float uAlpha;
uniform int uSection;
uniform vec4 uClip;
out vec4 frag;
void main(){
    if(uSection == 1 && dot(vW, uClip.xyz) > uClip.w) discard;
    frag = vec4(uColor, uAlpha);
}
)";

Vec3 Camera::eye() const {
    float cp = std::cos(pitch), sp = std::sin(pitch);
    float cy = std::cos(yaw), sy = std::sin(yaw);
    // Z-up: yaw about Z, pitch from horizon
    Vec3 dir{cp * cy, cp * sy, sp};
    return target + dir * distance;
}

Mat4 Camera::view() const { return Mat4::look_at(eye(), target, {0, 0, 1}); }

Mat4 Camera::proj() const {
    float zn = std::max(0.05f, distance * 0.002f);
    float zf = std::max(4000.f, distance * 20.f);
    if (ortho) {
        float h = distance * 0.45f;
        float ww = h * aspect;
        return Mat4::ortho(-ww, ww, -h, h, -zf, zf);
    }
    return Mat4::perspective(fovy, aspect, zn, zf);
}

Ray Camera::ray_from_pixel(float x, float y) const {
    float nx = (x / (float)w) * 2.f - 1.f;
    float ny = 1.f - (y / (float)h) * 2.f;
    Mat4 inv = (proj() * view()).inverse();
    Vec3 a = inv.transform_point({nx, ny, -1});
    Vec3 b = inv.transform_point({nx, ny, 1});
    Ray r;
    r.o = a;
    r.d = (b - a).normalized();
    if (ortho) {
        r.o = a;
        r.d = (target - eye()).normalized();
        // better: unproject both and use view forward
        Vec3 e = eye();
        r.o = a;
        r.d = (target - e).normalized();
    }
    return r;
}

void Camera::orbit(float dx, float dy) {
    yaw -= dx * 0.008f;
    pitch += dy * 0.008f;
    pitch = clamp(pitch, -1.52f, 1.52f);
}

void Camera::pan(float dx, float dy) {
    Vec3 e = eye();
    Vec3 f = (target - e).normalized();
    Vec3 r = f.cross({0, 0, 1});
    if (r.length2() < 1e-8f) r = f.cross({0, 1, 0});
    r = r.normalized();
    Vec3 u = r.cross(f).normalized();
    float s = distance * 0.0018f;
    target = target - r * dx * s + u * dy * s;
}

void Camera::zoom(float factor) { distance = clamp(distance * factor, 2.f, 20000.f); }

void Camera::frame(const Aabb& b) {
    if (!b.valid()) {
        target = {20, 15, 8};
        distance = 160;
        return;
    }
    target = b.center();
    distance = std::max(40.f, b.max_extent() * 2.1f);
}

void Camera::set_view(const char* name) {
    std::string n = name;
    if (n == "top") {
        yaw = -kPi * 0.5f;
        pitch = 1.52f;
        ortho = true;
    } else if (n == "front") {
        yaw = -kPi * 0.5f;
        pitch = 0;
        ortho = true;
    } else if (n == "right") {
        yaw = 0;
        pitch = 0;
        ortho = true;
    } else if (n == "left") {
        yaw = kPi;
        pitch = 0;
        ortho = true;
    } else if (n == "home" || n == "iso") {
        yaw = 35 * kDeg;
        pitch = 32 * kDeg;
        ortho = false;
    }
}

void GpuMesh::destroy() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (ibo) glDeleteBuffers(1, &ibo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = ibo = ebo = 0;
    nidx = nedge = 0;
}

void GpuMesh::upload(const Mesh& m) {
    if (!vao) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        glGenBuffers(1, &ebo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(m.vertices.size() * sizeof(Vertex)), m.vertices.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, feature_id));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, scalar));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(m.indices.size() * sizeof(std::uint32_t)), m.indices.data(),
                 GL_DYNAMIC_DRAW);
    nidx = (GLsizei)m.indices.size();
    glBindBuffer(GL_ARRAY_BUFFER, ebo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(m.edges.size() * sizeof(Vec3)), m.edges.data(), GL_DYNAMIC_DRAW);
    nedge = (GLsizei)m.edges.size();
    glBindVertexArray(0);
}

GLuint Renderer::compile(GLenum type, const char* src, std::string* err) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        if (err) *err = log;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint Renderer::link(GLuint vs, GLuint fs, std::string* err) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, 1024, nullptr, log);
        if (err) *err = log;
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

bool Renderer::init(std::string* err) {
    GLuint vs = compile(GL_VERTEX_SHADER, kSolidVS, err);
    if (!vs) return false;
    GLuint fs = compile(GL_FRAGMENT_SHADER, kSolidFS, err);
    if (!fs) return false;
    prog_solid = link(vs, fs, err);
    if (!prog_solid) return false;
    vs = compile(GL_VERTEX_SHADER, kLineVS, err);
    if (!vs) return false;
    fs = compile(GL_FRAGMENT_SHADER, kLineFS, err);
    if (!fs) return false;
    prog_line = link(vs, fs, err);
    if (!prog_line) return false;
    vs = compile(GL_VERTEX_SHADER, kTraceVS, err);
    if (!vs) return false;
    fs = compile(GL_FRAGMENT_SHADER, kTraceFS, err);
    if (!fs) return false;
    prog_trace = link(vs, fs, err);
    if (!prog_trace) return false;
    glGenVertexArrays(1, &vao_fs);
    glGenBuffers(1, &prim_bo);
    glGenBuffers(1, &curve_bo);
    glGenTextures(1, &prim_tex);
    glGenTextures(1, &curve_tex);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    return true;
}

void Renderer::shutdown() {
    body.destroy();
    preview.destroy();
    if (color) glDeleteTextures(1, &color);
    if (depth) glDeleteRenderbuffers(1, &depth);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (msaa_color) glDeleteRenderbuffers(1, &msaa_color);
    if (msaa_depth) glDeleteRenderbuffers(1, &msaa_depth);
    if (msaa_fbo) glDeleteFramebuffers(1, &msaa_fbo);
    if (ss_color) glDeleteRenderbuffers(1, &ss_color);
    if (ss_depth) glDeleteRenderbuffers(1, &ss_depth);
    if (ss_fbo) glDeleteFramebuffers(1, &ss_fbo);
    if (prog_solid) glDeleteProgram(prog_solid);
    if (prog_line) glDeleteProgram(prog_line);
    if (prog_trace) glDeleteProgram(prog_trace);
    if (vao_fs) glDeleteVertexArrays(1, &vao_fs);
    if (prim_tex) glDeleteTextures(1, &prim_tex);
    if (curve_tex) glDeleteTextures(1, &curve_tex);
    if (prim_bo) glDeleteBuffers(1, &prim_bo);
    if (curve_bo) glDeleteBuffers(1, &curve_bo);
}

void Renderer::resize(int w, int h) {
    w = std::max(4, w);
    h = std::max(4, h);
    if (w == fb_w && h == fb_h && fbo) return;
    fb_w = w;
    fb_h = h;
    if (!fbo) glGenFramebuffers(1, &fbo);
    if (!color) glGenTextures(1, &color);
    if (!depth) glGenRenderbuffers(1, &depth);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);

    if (!msaa_fbo) glGenFramebuffers(1, &msaa_fbo);
    if (!msaa_color) glGenRenderbuffers(1, &msaa_color);
    if (!msaa_depth) glGenRenderbuffers(1, &msaa_depth);
    GLint max_s = 4;
    glGetIntegerv(GL_MAX_SAMPLES, &max_s);
    msaa_samples = std::min(4, std::max(0, max_s));
    if (msaa_samples >= 2) {
        glBindRenderbuffer(GL_RENDERBUFFER, msaa_color);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_samples, GL_RGBA8, w, h);
        glBindRenderbuffer(GL_RENDERBUFFER, msaa_depth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_samples, GL_DEPTH24_STENCIL8, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaa_color);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaa_depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) msaa_samples = 0;
    }

    ss_scale = 3;
    ss_w = w * ss_scale;
    ss_h = h * ss_scale;
    if (ss_w > 7680 || ss_h > 7680) {
        ss_scale = 2;
        ss_w = w * ss_scale;
        ss_h = h * ss_scale;
    }
    if (ss_w > 8192 || ss_h > 8192) {
        ss_scale = 1;
        ss_w = w;
        ss_h = h;
    }
    if (!ss_fbo) glGenFramebuffers(1, &ss_fbo);
    if (!ss_color) glGenRenderbuffers(1, &ss_color);
    if (!ss_depth) glGenRenderbuffers(1, &ss_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, ss_color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, ss_w, ss_h);
    glBindRenderbuffer(GL_RENDERBUFFER, ss_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ss_w, ss_h);
    glBindFramebuffer(GL_FRAMEBUFFER, ss_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, ss_color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ss_depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        ss_scale = 1;
        ss_w = w;
        ss_h = h;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static GLuint g_line_vao = 0, g_line_vbo = 0;

static void ensure_line_buf() {
    if (g_line_vao) return;
    glGenVertexArrays(1, &g_line_vao);
    glGenBuffers(1, &g_line_vbo);
    glBindVertexArray(g_line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), nullptr);
    glBindVertexArray(0);
}

void Renderer::draw_lines(const Camera& cam, const std::vector<Vec3>& pts, const Vec3& color, float width) {
    if (pts.empty()) return;
    ensure_line_buf();
    Mat4 vp = cam.proj() * cam.view();
    glUseProgram(prog_line);
    glUniformMatrix4fv(glGetUniformLocation(prog_line, "uVP"), 1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(prog_line, "uColor"), color.x, color.y, color.z);
    glUniform1f(glGetUniformLocation(prog_line, "uAlpha"), 1.f);
    glUniform1i(glGetUniformLocation(prog_line, "uSection"), section ? 1 : 0);
    glUniform4f(glGetUniformLocation(prog_line, "uClip"), section_n.x, section_n.y, section_n.z, section_d);
    glBindVertexArray(g_line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(pts.size() * sizeof(Vec3)), pts.data(), GL_STREAM_DRAW);
    glLineWidth(width);
    glDrawArrays(GL_LINES, 0, (GLsizei)pts.size());
    glBindVertexArray(0);
}

void Renderer::draw_grid(const Camera& cam) {
    std::vector<Vec3> minor, major, axisx, axisy;
    const float span = std::max(200.f, cam.distance * 1.8f);
    float step = 10.f;
    if (cam.distance > 400) step = 50;
    if (cam.distance > 1200) step = 100;
    float major_every = step * 5;
    int n = (int)(span / step);
    for (int i = -n; i <= n; ++i) {
        float t = i * step;
        Vec3 a{t, -span, 0}, b{t, span, 0}, c{-span, t, 0}, d{span, t, 0};
        bool is_major = (std::fabs(std::fmod(t, major_every)) < 0.01f);
        if (std::fabs(t) < 0.01f) {
            axisy.push_back(a);
            axisy.push_back(b);
            axisx.push_back(c);
            axisx.push_back(d);
        } else if (is_major) {
            major.push_back(a);
            major.push_back(b);
            major.push_back(c);
            major.push_back(d);
        } else {
            minor.push_back(a);
            minor.push_back(b);
            minor.push_back(c);
            minor.push_back(d);
        }
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    Mat4 vp = cam.proj() * cam.view();
    glUseProgram(prog_line);
    glUniformMatrix4fv(glGetUniformLocation(prog_line, "uVP"), 1, GL_FALSE, vp.m);
    glUniform1i(glGetUniformLocation(prog_line, "uSection"), 0);
    ensure_line_buf();
    glBindVertexArray(g_line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);
    auto batch = [&](const std::vector<Vec3>& pts, Vec3 col, float a) {
        if (pts.empty()) return;
        glUniform3f(glGetUniformLocation(prog_line, "uColor"), col.x, col.y, col.z);
        glUniform1f(glGetUniformLocation(prog_line, "uAlpha"), a);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(pts.size() * sizeof(Vec3)), pts.data(), GL_STREAM_DRAW);
        glDrawArrays(GL_LINES, 0, (GLsizei)pts.size());
    };
    batch(minor, {0.48f, 0.52f, 0.58f}, 0.55f);
    batch(major, {0.62f, 0.66f, 0.72f}, 0.75f);
    batch(axisx, {0.82f, 0.28f, 0.28f}, 0.85f);
    batch(axisy, {0.28f, 0.72f, 0.38f}, 0.85f);
    glDepthMask(GL_TRUE);
}

void Renderer::draw_planes() {
    // translucent workplanes drawn as lines (quads would need another shader; line frames are clearer)
}

void Renderer::draw_origin() {
    std::vector<Vec3> x{{0, 0, 0}, {28, 0, 0}};
    std::vector<Vec3> y{{0, 0, 0}, {0, 28, 0}};
    std::vector<Vec3> z{{0, 0, 0}, {0, 0, 28}};
    // drawn by caller with camera
    (void)x;
    (void)y;
    (void)z;
}

GLuint Renderer::draw(const Camera& cam_in, const Document& doc, const Mesh* preview_mesh, int active_sketch_plane,
                      Vec3 sketch_origin, const std::vector<Vec3>& extra_lines, const std::vector<Vec3>& extra_colors) {
    bool hi = implicit && !analysis && doc.components.empty() && !doc.scene.nodes.empty() &&
              shading != Shading::HiddenLine && ss_fbo && ss_scale >= 2 && !doc.has_imports();
    Camera cam = cam_in;
    int dw = hi ? ss_w : fb_w;
    int dh = hi ? ss_h : fb_h;
    cam.w = dw;
    cam.h = dh;
    cam.aspect = dh > 0 ? (float)dw / (float)dh : 1.f;

    if (doc.rev != uploaded_rev) {
        body.upload(doc.body);
        uploaded_rev = doc.rev;
    }
    if (preview_mesh && !preview_mesh->empty()) {
        preview.upload(*preview_mesh);
        preview_valid = true;
    } else {
        preview_valid = false;
    }

    GLuint target = hi ? ss_fbo : (msaa_samples >= 2 ? msaa_fbo : fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glViewport(0, 0, dw, dh);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.145f, 0.165f, 0.188f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (show_grid) draw_grid(cam);

    // Origin triad
    if (show_origin) {
        float lw = hi ? 3.2f : 2.2f;
        draw_lines(cam, {{0, 0, 0}, {32, 0, 0}}, {0.90f, 0.30f, 0.30f}, lw);
        draw_lines(cam, {{0, 0, 0}, {0, 32, 0}}, {0.32f, 0.78f, 0.40f}, lw);
        draw_lines(cam, {{0, 0, 0}, {0, 0, 32}}, {0.30f, 0.55f, 0.95f}, lw);
    }

    if (show_planes) {
        auto frame = [&](int pl, Vec3 col) {
            Vec3 u, v, n;
            plane_basis(pl, u, v, n);
            float s = 45;
            Vec3 o{0, 0, 0};
            std::vector<Vec3> q{o, o + u * s, o + u * s, o + u * s + v * s, o + u * s + v * s, o + v * s, o + v * s, o};
            draw_lines(cam, q, col, 1.2f);
        };
        frame(0, {0.35f, 0.55f, 0.85f});
        frame(1, {0.85f, 0.45f, 0.35f});
        frame(2, {0.40f, 0.80f, 0.50f});
    }

    if (active_sketch_plane >= 0) {
        Vec3 u, v, n;
        plane_basis(active_sketch_plane, u, v, n);
        float s = 80;
        Vec3 o = sketch_origin;
        std::vector<Vec3> q{o - u * s - v * s, o + u * s - v * s, o + u * s - v * s, o + u * s + v * s,
                            o + u * s + v * s, o - u * s + v * s, o - u * s + v * s, o - u * s - v * s};
        draw_lines(cam, q, {0.20f, 0.85f, 0.95f}, 1.8f);
    }

    Mat4 vp = cam.proj() * cam.view();
    Vec3 eye = cam.eye();

    auto draw_gpu = [&](GpuMesh& gm, Vec3 col, bool previewing) {
        if (!gm.nidx) return;
        glUseProgram(prog_solid);
        glUniformMatrix4fv(glGetUniformLocation(prog_solid, "uVP"), 1, GL_FALSE, vp.m);
        glUniform3f(glGetUniformLocation(prog_solid, "uEye"), eye.x, eye.y, eye.z);
        glUniform3f(glGetUniformLocation(prog_solid, "uColor"), col.x, col.y, col.z);
        glUniform1ui(glGetUniformLocation(prog_solid, "uSel"), selected_id);
        glUniform1ui(glGetUniformLocation(prog_solid, "uHover"), hover_id);
        glUniform1i(glGetUniformLocation(prog_solid, "uWire"), shading == Shading::Wire || shading == Shading::HiddenLine ? 1 : 0);
        glUniform1i(glGetUniformLocation(prog_solid, "uSection"), section ? 1 : 0);
        glUniform1i(glGetUniformLocation(prog_solid, "uAnalysis"), analysis ? 1 : 0);
        glUniform4f(glGetUniformLocation(prog_solid, "uClip"), section_n.x, section_n.y, section_n.z, section_d);
        glBindVertexArray(gm.vao);
        if (shading == Shading::Wire) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, gm.nidx, GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            if (previewing) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }
            glDrawElements(GL_TRIANGLES, gm.nidx, GL_UNSIGNED_INT, nullptr);
            glDisable(GL_CULL_FACE);
        }
        if ((shading == Shading::ShadedEdges || shading == Shading::HiddenLine) && gm.nedge) {
            glUseProgram(prog_line);
            glUniformMatrix4fv(glGetUniformLocation(prog_line, "uVP"), 1, GL_FALSE, vp.m);
            glUniform3f(glGetUniformLocation(prog_line, "uColor"), 0.08f, 0.09f, 0.10f);
            glUniform1f(glGetUniformLocation(prog_line, "uAlpha"), 0.95f);
            glUniform1i(glGetUniformLocation(prog_line, "uSection"), section ? 1 : 0);
            glUniform4f(glGetUniformLocation(prog_line, "uClip"), section_n.x, section_n.y, section_n.z, section_d);
            glBindBuffer(GL_ARRAY_BUFFER, gm.ebo);
            ensure_line_buf();
            glBindVertexArray(g_line_vao);
            glBindBuffer(GL_ARRAY_BUFFER, gm.ebo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), nullptr);
            glLineWidth(1.4f);
            glDrawArrays(GL_LINES, 0, gm.nedge);
        }
        glBindVertexArray(0);
    };

    Vec3 col = analysis ? Vec3{1, 1, 1} : material_at(doc.material_id).color;
    bool used_implicit = false;
    if (implicit && !analysis && doc.components.empty() && !doc.scene.nodes.empty() &&
        shading != Shading::HiddenLine && !doc.has_imports()) {
        used_implicit = draw_implicit(cam, doc, col);
    }
    if (!used_implicit) {
        if (shading == Shading::HiddenLine) {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            draw_gpu(body, body_color, false);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glUseProgram(prog_solid);
            glUniform1i(glGetUniformLocation(prog_solid, "uWire"), 0);
            glBindVertexArray(body.vao);
            glUniform3f(glGetUniformLocation(prog_solid, "uColor"), 0.92f, 0.93f, 0.94f);
            glDrawElements(GL_TRIANGLES, body.nidx, GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            draw_gpu(body, col, false);
        }
    }

    if (preview_mesh && !preview_mesh->empty()) {
        glEnable(GL_BLEND);
        draw_gpu(preview, {1.0f, 0.62f, 0.18f}, true);
    }

    if (!extra_lines.empty()) {
        if (extra_colors.size() >= extra_lines.size() / 2) {
            size_t i = 0;
            while (i + 1 < extra_lines.size()) {
                Vec3 col = extra_colors[i / 2];
                std::vector<Vec3> batch;
                while (i + 1 < extra_lines.size()) {
                    Vec3 c = extra_colors[i / 2];
                    if ((c - col).length2() > 1e-5f) break;
                    batch.push_back(extra_lines[i]);
                    batch.push_back(extra_lines[i + 1]);
                    i += 2;
                }
                draw_lines(cam, batch, col, 2.2f);
            }
        } else {
            Vec3 col = extra_colors.empty() ? Vec3{1.f, 0.85f, 0.2f} : extra_colors[0];
            draw_lines(cam, extra_lines, col, 2.0f);
        }
    }

    if (hi) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, ss_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, ss_w, ss_h, 0, 0, fb_w, fb_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    } else if (msaa_samples >= 2) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, fb_w, fb_h, 0, 0, fb_w, fb_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return color;
}

void Renderer::upload_scene(const Document& doc) {
    if (doc.rev == scene_rev && nprims > 0) return;
    scene_rev = doc.rev;
    const SdfScene& sc = doc.scene;
    nprims = 0;
    std::vector<float> prims;
    std::vector<float> curves;
    auto push4 = [&](std::vector<float>& dst, float a, float b, float c, float d) {
        dst.push_back(a);
        dst.push_back(b);
        dst.push_back(c);
        dst.push_back(d);
    };
    auto add_curve2 = [&](const std::vector<Vec2>& pts) -> std::pair<int, int> {
        int off = (int)curves.size() / 4;
        for (auto q : pts) push4(curves, q.x, q.y, 0, 0);
        return {off, (int)pts.size()};
    };
    auto add_curve3 = [&](const std::vector<Vec3>& pts) -> std::pair<int, int> {
        int off = (int)curves.size() / 4;
        for (auto q : pts) push4(curves, q.x, q.y, q.z, 0);
        return {off, (int)pts.size()};
    };
    const int maxn = 48;
    for (const auto& node : sc.nodes) {
        if (nprims >= maxn) break;
        auto pr = add_curve2(node.prim.profile);
        auto pb = add_curve2(node.prim.profile_b);
        auto pa = add_curve3(node.prim.path);
        // 16 texels
        push4(prims, (float)node.prim.kind, (float)node.op, node.blend, node.chamfer ? 1.f : 0.f);
        push4(prims, node.prim.size.x, node.prim.size.y, node.prim.size.z, node.prim.height);
        push4(prims, node.prim.origin.x, node.prim.origin.y, node.prim.origin.z, node.prim.round);
        push4(prims, node.prim.euler_deg.x, node.prim.euler_deg.y, node.prim.euler_deg.z, node.prim.taper);
        push4(prims, (float)node.prim.style, (float)node.prim.plane, (float)node.prim.feature_id, 0);
        push4(prims, (float)pr.first, (float)pr.second, (float)pa.first, (float)pa.second);
        push4(prims, (float)pb.first, (float)pb.second, 0, 0);
        Aabb bb = node.prim.aabb.valid() ? node.prim.aabb : bounds_prim(node.prim);
        push4(prims, bb.mn.x, bb.mn.y, bb.mn.z, 0);
        push4(prims, bb.mx.x, bb.mx.y, bb.mx.z, 0);
        const float* m = node.prim.inv.m;
        push4(prims, m[0], m[4], m[8], m[12]);
        push4(prims, m[1], m[5], m[9], m[13]);
        push4(prims, m[2], m[6], m[10], m[14]);
        push4(prims, m[3], m[7], m[11], m[15]);
        Vec3 n2e = node.prim.path.empty() ? Vec3{} : node.prim.path[0];
        push4(prims, n2e.x, n2e.y, n2e.z, 0);
        push4(prims, 0, 0, 0, 0);
        push4(prims, 0, 0, 0, 0);
        ++nprims;
    }
    if (curves.empty()) push4(curves, 0, 0, 0, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, prim_bo);
    glBufferData(GL_TEXTURE_BUFFER, (GLsizeiptr)(prims.size() * sizeof(float)), prims.data(), GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, prim_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, prim_bo);
    glBindBuffer(GL_TEXTURE_BUFFER, curve_bo);
    glBufferData(GL_TEXTURE_BUFFER, (GLsizeiptr)(curves.size() * sizeof(float)), curves.data(), GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, curve_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, curve_bo);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

bool Renderer::draw_implicit(const Camera& cam, const Document& doc, Vec3 color) {
    if (!prog_trace || doc.scene.nodes.empty()) return false;
    upload_scene(doc);
    if (nprims <= 0) return false;
    Mat4 vp = cam.proj() * cam.view();
    Mat4 inv = vp.inverse();
    glUseProgram(prog_trace);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, prim_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, curve_tex);
    glUniform1i(glGetUniformLocation(prog_trace, "uPrims"), 0);
    glUniform1i(glGetUniformLocation(prog_trace, "uCurves"), 1);
    glUniform1i(glGetUniformLocation(prog_trace, "uN"), nprims);
    Vec3 eye = cam.eye();
    glUniform3f(glGetUniformLocation(prog_trace, "uEye"), eye.x, eye.y, eye.z);
    glUniformMatrix4fv(glGetUniformLocation(prog_trace, "uInvVP"), 1, GL_FALSE, inv.m);
    glUniformMatrix4fv(glGetUniformLocation(prog_trace, "uVP"), 1, GL_FALSE, vp.m);
    glUniform2f(glGetUniformLocation(prog_trace, "uRes"), (float)cam.w, (float)cam.h);
    glUniform3f(glGetUniformLocation(prog_trace, "uColor"), color.x, color.y, color.z);
    glUniform1ui(glGetUniformLocation(prog_trace, "uSel"), selected_id);
    glUniform1ui(glGetUniformLocation(prog_trace, "uHover"), hover_id);
    glUniform1i(glGetUniformLocation(prog_trace, "uSection"), section ? 1 : 0);
    glUniform4f(glGetUniformLocation(prog_trace, "uClip"), section_n.x, section_n.y, section_n.z, section_d);
    float farp = std::max(400.f, cam.distance * 8.f);
    glUniform1f(glGetUniformLocation(prog_trace, "uFar"), farp);
    Aabb bb = bounds_scene(doc.scene, -1);
    if (!bb.valid()) bb = doc.body.bounds;
    bb = bb.padded(std::max(1.5f, bb.max_extent() * 0.04f));
    glUniform3f(glGetUniformLocation(prog_trace, "uBMin"), bb.mn.x, bb.mn.y, bb.mn.z);
    glUniform3f(glGetUniformLocation(prog_trace, "uBMax"), bb.mx.x, bb.mx.y, bb.mx.z);
    glUniform1i(glGetUniformLocation(prog_trace, "uWire"), shading == Shading::Wire ? 1 : 0);
    glUniform1i(glGetUniformLocation(prog_trace, "uContours"), shading == Shading::ShadedEdges ? 1 : 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glBindVertexArray(vao_fs);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    return true;
}

} // namespace ax
