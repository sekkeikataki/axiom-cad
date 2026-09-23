#include "icons.hpp"
#include "document.hpp"
#include "app.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace ax {

static IconAtlas g_atlas;

IconAtlas& icons() { return g_atlas; }

struct Canvas {
    std::vector<std::uint8_t>* buf = nullptr;
    int tw = 0, th = 0, cell = 64, ox = 0, oy = 0;

    void pset(int x, int y, std::uint32_t c, float a = 1.f) {
        x += ox;
        y += oy;
        if (x < ox || y < oy || x >= ox + cell || y >= oy + cell) return;
        if (x < 0 || y < 0 || x >= tw || y >= th) return;
        a = clamp(a, 0.f, 1.f);
        std::uint8_t r = (c >> 24) & 255, g = (c >> 16) & 255, b = (c >> 8) & 255, al = c & 255;
        size_t i = ((size_t)y * tw + x) * 4;
        float srca = (al / 255.f) * a;
        float da = (*buf)[i + 3] / 255.f;
        float outa = srca + da * (1 - srca);
        if (outa < 1e-4f) return;
        auto blend = [&](int ch, std::uint8_t s) {
            float d = (*buf)[i + ch] / 255.f;
            float o = (s / 255.f * srca + d * da * (1 - srca)) / outa;
            (*buf)[i + ch] = (std::uint8_t)std::lround(clamp(o, 0.f, 1.f) * 255.f);
        };
        blend(0, r);
        blend(1, g);
        blend(2, b);
        (*buf)[i + 3] = (std::uint8_t)std::lround(outa * 255.f);
    }

    void line(float x0, float y0, float x1, float y1, std::uint32_t c, float w = 1.4f) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.01f) {
            pset((int)x0, (int)y0, c);
            return;
        }
        int n = (int)std::ceil(len * 2.f) + 1;
        for (int i = 0; i <= n; ++i) {
            float t = (float)i / (float)n;
            float x = x0 + dx * t, y = y0 + dy * t;
            for (int oy = -1; oy <= 1; ++oy)
                for (int ox = -1; ox <= 1; ++ox) {
                    float px = x + ox * 0.35f, py = y + oy * 0.35f;
                    float d = std::fabs((px - x0) * dy - (py - y0) * dx) / len;
                    float a = clamp(1.f - (d / w), 0.f, 1.f);
                    if (a > 0.02f) pset((int)std::lround(px), (int)std::lround(py), c, a);
                }
        }
    }

    void tri(float ax, float ay, float bx, float by, float cx, float cy, std::uint32_t col) {
        int minx = (int)std::floor(std::min(ax, std::min(bx, cx)));
        int maxx = (int)std::ceil(std::max(ax, std::max(bx, cx)));
        int miny = (int)std::floor(std::min(ay, std::min(by, cy)));
        int maxy = (int)std::ceil(std::max(ay, std::max(by, cy)));
        auto edge = [](float x0, float y0, float x1, float y1, float x, float y) {
            return (x - x0) * (y1 - y0) - (y - y0) * (x1 - x0);
        };
        float area = edge(ax, ay, bx, by, cx, cy);
        if (std::fabs(area) < 1e-4f) return;
        for (int y = miny; y <= maxy; ++y)
            for (int x = minx; x <= maxx; ++x) {
                float w0 = edge(bx, by, cx, cy, (float)x + 0.5f, (float)y + 0.5f);
                float w1 = edge(cx, cy, ax, ay, (float)x + 0.5f, (float)y + 0.5f);
                float w2 = edge(ax, ay, bx, by, (float)x + 0.5f, (float)y + 0.5f);
                if ((w0 >= -0.5f && w1 >= -0.5f && w2 >= -0.5f && area > 0) ||
                    (w0 <= 0.5f && w1 <= 0.5f && w2 <= 0.5f && area < 0))
                    pset(x, y, col);
            }
        line(ax, ay, bx, by, 0x1A1610FFu, 1.1f);
        line(bx, by, cx, cy, 0x1A1610FFu, 1.1f);
        line(cx, cy, ax, ay, 0x1A1610FFu, 1.1f);
    }

    void quad(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy, std::uint32_t col) {
        tri(ax, ay, bx, by, cx, cy, col);
        tri(ax, ay, cx, cy, dx, dy, col);
    }

    void disc(float cx, float cy, float r, std::uint32_t col, bool fill = true) {
        int ir = (int)std::ceil(r) + 1;
        for (int y = -ir; y <= ir; ++y)
            for (int x = -ir; x <= ir; ++x) {
                float d = std::sqrt((float)(x * x + y * y));
                if (fill && d <= r) pset((int)cx + x, (int)cy + y, col, clamp(r - d + 1.f, 0.f, 1.f));
                else if (!fill && std::fabs(d - r) < 1.3f)
                    pset((int)cx + x, (int)cy + y, col, 1.f - std::fabs(d - r));
            }
    }

    // Isometric: x right-down, y left-down, z up. Origin near bottom-center of cell.
    void iso(float x, float y, float z, float& sx, float& sy) const {
        sx = 32.f + (x - y) * 13.5f;
        sy = 42.f + (x + y) * 7.8f - z * 16.f;
    }
};

static const std::uint32_t kTop = 0xF4D56AFF;
static const std::uint32_t kLft = 0xB8862BFF;
static const std::uint32_t kRgt = 0xD4A83AFF;
static const std::uint32_t kInk = 0x1E1A14FF;
static const std::uint32_t kBlue = 0x4FA3F7FF;
static const std::uint32_t kBlueDk = 0x1E5A9AFF;
static const std::uint32_t kGreen = 0x6FCB6AFF;
static const std::uint32_t kSteel = 0x9AA4B0FF;
static const std::uint32_t kWhite = 0xF4F6F8FF;
static const std::uint32_t kRed = 0xE24B3CFF;
static const std::uint32_t kOrange = 0xE8942AFF;

static void iso_box(Canvas& c, float x, float y, float z, float w, float d, float h, std::uint32_t top, std::uint32_t l,
                    std::uint32_t r) {
    auto P = [&](float px, float py, float pz, float& sx, float& sy) { c.iso(x + px, y + py, z + pz, sx, sy); };
    float s[8][2];
    // 0:000 1:w00 2:wd0 3:0d0  4:00h 5:w0h 6:wdh 7:0dh
    P(0, 0, 0, s[0][0], s[0][1]);
    P(w, 0, 0, s[1][0], s[1][1]);
    P(w, d, 0, s[2][0], s[2][1]);
    P(0, d, 0, s[3][0], s[3][1]);
    P(0, 0, h, s[4][0], s[4][1]);
    P(w, 0, h, s[5][0], s[5][1]);
    P(w, d, h, s[6][0], s[6][1]);
    P(0, d, h, s[7][0], s[7][1]);
    c.quad(s[4][0], s[4][1], s[5][0], s[5][1], s[6][0], s[6][1], s[7][0], s[7][1], top);
    c.quad(s[0][0], s[0][1], s[4][0], s[4][1], s[7][0], s[7][1], s[3][0], s[3][1], l);
    c.quad(s[1][0], s[1][1], s[2][0], s[2][1], s[6][0], s[6][1], s[5][0], s[5][1], r);
}

static void paint(Canvas& c, Icon id) {
    switch (id) {
    case Icon::New: {
        c.quad(18, 10, 46, 10, 46, 50, 18, 50, kWhite);
        c.tri(34, 10, 46, 10, 46, 22, kSteel);
        c.line(24, 28, 40, 28, kBlue, 1.6f);
        c.line(24, 36, 40, 36, kBlue, 1.6f);
        c.line(24, 44, 34, 44, kBlue, 1.6f);
        break;
    }
    case Icon::Open: {
        c.quad(12, 28, 50, 28, 46, 50, 16, 50, kRgt);
        c.quad(14, 18, 32, 18, 36, 28, 12, 28, kTop);
        c.quad(16, 24, 52, 20, 48, 32, 14, 32, 0xE8C24AFF);
        break;
    }
    case Icon::Save: {
        c.quad(16, 12, 48, 12, 50, 50, 14, 50, kBlue);
        c.quad(22, 12, 42, 12, 42, 28, 22, 28, kWhite);
        c.disc(32, 40, 6, kTop);
        break;
    }
    case Icon::Select: {
        c.tri(18, 12, 22, 48, 30, 36, kWhite);
        c.tri(22, 48, 30, 36, 38, 52, kSteel);
        c.line(34, 34, 50, 50, kTop, 2.2f);
        break;
    }
    case Icon::Box:
        iso_box(c, -0.7f, -0.55f, 0.1f, 1.4f, 1.15f, 1.25f, kTop, kLft, kRgt);
        break;
    case Icon::Cylinder: {
        // isometric cylinder via stacked ellipses
        for (int i = 0; i < 18; ++i) {
            float t0 = kTau * i / 18.f, t1 = kTau * (i + 1) / 18.f;
            float x0 = std::cos(t0), y0 = std::sin(t0), x1 = std::cos(t1), y1 = std::sin(t1);
            float ax, ay, bx, by, cx, cy, dx, dy;
            c.iso(x0 * 0.85f, y0 * 0.85f, 0.05f, ax, ay);
            c.iso(x1 * 0.85f, y1 * 0.85f, 0.05f, bx, by);
            c.iso(x1 * 0.85f, y1 * 0.85f, 1.55f, cx, cy);
            c.iso(x0 * 0.85f, y0 * 0.85f, 1.55f, dx, dy);
            std::uint32_t col = (x0 + y0 > 0) ? kRgt : kLft;
            c.quad(ax, ay, bx, by, cx, cy, dx, dy, col);
        }
        {
            float pts[18][2];
            for (int i = 0; i < 18; ++i) {
                float t = kTau * i / 18.f;
                c.iso(std::cos(t) * 0.85f, std::sin(t) * 0.85f, 1.55f, pts[i][0], pts[i][1]);
            }
            for (int i = 1; i < 17; ++i)
                c.tri(pts[0][0], pts[0][1], pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1], kTop);
        }
        break;
    }
    case Icon::Sphere: {
        c.disc(32, 30, 20, kRgt);
        c.disc(28, 26, 16, kTop);
        c.disc(24, 22, 6, 0xFFF3C8FF);
        c.disc(32, 30, 20, kInk, false);
        break;
    }
    case Icon::Cone: {
        float ax, ay, bx, by, cx, cy, tx, ty;
        c.iso(0, 0, 1.7f, tx, ty);
        for (int i = 0; i < 16; ++i) {
            float t0 = kTau * i / 16.f, t1 = kTau * (i + 1) / 16.f;
            c.iso(std::cos(t0), std::sin(t0), 0.1f, ax, ay);
            c.iso(std::cos(t1), std::sin(t1), 0.1f, bx, by);
            c.tri(tx, ty, ax, ay, bx, by, (std::cos(t0) > 0) ? kRgt : kLft);
        }
        break;
    }
    case Icon::Hole: {
        iso_box(c, -0.9f, -0.7f, 0.2f, 1.8f, 1.4f, 0.45f, kTop, kLft, kRgt);
        float cx, cy;
        c.iso(0.0f, 0.0f, 0.7f, cx, cy);
        c.disc(cx, cy - 2, 7, 0x2A2218FF);
        c.disc(cx - 1, cy - 4, 4, 0x1A1510FF);
        break;
    }
    case Icon::Extrude: {
        c.quad(16, 40, 48, 40, 48, 50, 16, 50, kBlueDk);
        c.quad(18, 42, 30, 42, 30, 48, 18, 48, kBlue);
        iso_box(c, -0.45f, -0.35f, 0.55f, 0.95f, 0.75f, 1.05f, kTop, kLft, kRgt);
        c.line(32, 8, 32, 22, kGreen, 2.4f);
        c.tri(32, 6, 26, 16, 38, 16, kGreen);
        break;
    }
    case Icon::Revolve: {
        c.line(18, 12, 18, 52, kInk, 1.8f);
        c.quad(20, 22, 40, 18, 42, 44, 22, 46, kBlue);
        for (int i = 0; i < 10; ++i) {
            float a0 = -0.2f + i * 0.22f, a1 = a0 + 0.22f;
            float x0 = 18 + std::cos(a0) * 22, y0 = 34 + std::sin(a0) * 10;
            float x1 = 18 + std::cos(a1) * 22, y1 = 34 + std::sin(a1) * 10;
            c.line(x0, y0, x1, y1, kTop, 1.5f);
        }
        break;
    }
    case Icon::Fillet: {
        c.quad(14, 38, 50, 38, 50, 50, 14, 50, kRgt);
        c.quad(14, 14, 26, 14, 26, 38, 14, 38, kLft);
        for (int i = 0; i <= 10; ++i) {
            float t = (kPi * 0.5f) * i / 10.f;
            float x = 26 + std::cos(t) * 16, y = 38 - std::sin(t) * 16;
            c.pset((int)x, (int)y, kTop);
            if (i > 0) {
                float t0 = (kPi * 0.5f) * (i - 1) / 10.f;
                c.line(26 + std::cos(t0) * 16, 38 - std::sin(t0) * 16, x, y, kTop, 2.2f);
            }
        }
        break;
    }
    case Icon::Chamfer: {
        c.quad(14, 40, 50, 40, 50, 52, 14, 52, kRgt);
        c.quad(14, 14, 26, 14, 14, 40, 14, 40, kLft);
        c.quad(26, 14, 50, 40, 40, 40, 26, 26, kTop);
        c.line(26, 14, 50, 40, kInk, 2.0f);
        break;
    }
    case Icon::RectPattern: {
        iso_box(c, -1.15f, -0.9f, 0.3f, 0.7f, 0.55f, 0.55f, kTop, kLft, kRgt);
        iso_box(c, -0.15f, -0.9f, 0.3f, 0.7f, 0.55f, 0.55f, kTop, kLft, kRgt);
        iso_box(c, -1.15f, 0.05f, 0.3f, 0.7f, 0.55f, 0.55f, kTop, kLft, kRgt);
        iso_box(c, -0.15f, 0.05f, 0.3f, 0.7f, 0.55f, 0.55f, kTop, kLft, kRgt);
        break;
    }
    case Icon::CircPattern: {
        for (int i = 0; i < 5; ++i) {
            float a = kTau * i / 5.f;
            iso_box(c, std::cos(a) * 0.85f - 0.28f, std::sin(a) * 0.85f - 0.22f, 0.35f, 0.5f, 0.42f, 0.5f, kTop, kLft,
                    kRgt);
        }
        break;
    }
    case Icon::Mirror: {
        iso_box(c, -1.15f, -0.4f, 0.2f, 0.85f, 0.7f, 1.1f, kTop, kLft, kRgt);
        c.line(32, 10, 32, 54, kBlue, 1.6f);
        iso_box(c, 0.3f, -0.4f, 0.2f, 0.85f, 0.7f, 1.1f, 0xC8C8D0FF, 0x888898FF, 0xA8A8B8FF);
        break;
    }
    case Icon::SketchRect: {
        c.quad(16, 16, 48, 16, 48, 48, 16, 48, 0x00000000);
        c.line(16, 16, 48, 16, kBlue, 2.0f);
        c.line(48, 16, 48, 48, kBlue, 2.0f);
        c.line(48, 48, 16, 48, kBlue, 2.0f);
        c.line(16, 48, 16, 16, kBlue, 2.0f);
        c.disc(16, 16, 3, kWhite);
        c.disc(48, 16, 3, kWhite);
        c.disc(48, 48, 3, kWhite);
        c.disc(16, 48, 3, kWhite);
        break;
    }
    case Icon::SketchCircle: {
        c.disc(32, 32, 18, kBlue, false);
        c.disc(32, 32, 3, kWhite);
        c.line(32, 32, 50, 32, kBlue, 1.4f);
        break;
    }
    case Icon::SketchLine: {
        c.line(14, 48, 50, 16, kBlue, 2.4f);
        c.disc(14, 48, 4, kWhite);
        c.disc(50, 16, 4, kWhite);
        break;
    }
    case Icon::Measure: {
        c.line(14, 46, 50, 18, kInk, 1.5f);
        c.line(18, 40, 22, 52, kGreen, 1.8f);
        c.line(42, 12, 46, 24, kGreen, 1.8f);
        c.line(22, 44, 44, 22, kGreen, 1.6f);
        break;
    }
    case Icon::Section: {
        iso_box(c, -0.7f, -0.55f, 0.15f, 1.4f, 1.15f, 1.2f, kTop, kLft, kRgt);
        c.quad(12, 8, 52, 28, 52, 36, 12, 16, 0x4FA3F7AA);
        break;
    }
    case Icon::Place: {
        iso_box(c, -0.85f, -0.3f, 0.1f, 1.1f, 0.9f, 0.45f, kTop, kLft, kRgt);
        iso_box(c, -0.55f, -0.2f, 0.6f, 0.85f, 0.7f, 0.7f, 0x8EC4F0FF, 0x3A6A9AFF, 0x5A8AB8FF);
        break;
    }
    case Icon::MateInsert: {
        iso_box(c, -0.7f, -0.5f, 0.15f, 1.4f, 1.0f, 0.35f, kSteel, 0x5A6270FF, 0x7A8490FF);
        for (int i = 0; i < 12; ++i) {
            float t0 = kTau * i / 12.f, t1 = kTau * (i + 1) / 12.f;
            float ax, ay, bx, by, cx, cy, dx, dy;
            c.iso(std::cos(t0) * 0.35f, std::sin(t0) * 0.35f, 0.5f, ax, ay);
            c.iso(std::cos(t1) * 0.35f, std::sin(t1) * 0.35f, 0.5f, bx, by);
            c.iso(std::cos(t1) * 0.35f, std::sin(t1) * 0.35f, 1.5f, cx, cy);
            c.iso(std::cos(t0) * 0.35f, std::sin(t0) * 0.35f, 1.5f, dx, dy);
            c.quad(ax, ay, bx, by, cx, cy, dx, dy, kRgt);
        }
        break;
    }
    case Icon::MateFlush: {
        iso_box(c, -0.9f, -0.5f, 0.1f, 1.8f, 1.0f, 0.4f, kTop, kLft, kRgt);
        iso_box(c, -0.9f, -0.5f, 0.7f, 1.8f, 1.0f, 0.4f, 0x8EC4F0FF, 0x3A6A9AFF, 0x5A8AB8FF);
        break;
    }
    case Icon::Explode: {
        iso_box(c, -1.2f, -0.2f, 0.1f, 0.8f, 0.7f, 0.5f, kTop, kLft, kRgt);
        iso_box(c, 0.2f, -0.2f, 0.9f, 0.8f, 0.7f, 0.5f, 0x8EC4F0FF, 0x3A6A9AFF, 0x5A8AB8FF);
        c.line(22, 40, 44, 20, kGreen, 2.0f);
        break;
    }
    case Icon::Stress: {
        iso_box(c, -0.7f, -0.5f, 0.15f, 1.4f, 1.05f, 1.15f, 0x3D6BFFFF, 0x1E3A8AFF, 0xE23A2AFF);
        c.quad(18, 18, 46, 18, 46, 28, 18, 22, 0xF0C040FF);
        break;
    }
    case Icon::Material: {
        iso_box(c, -0.5f, -0.4f, 0.2f, 1.0f, 0.8f, 1.2f, 0xC9A06AFF, 0x7A5A30FF, 0xA07840FF);
        c.disc(40, 20, 8, kSteel);
        break;
    }
    case Icon::Fit: {
        c.quad(16, 16, 48, 16, 48, 48, 16, 48, 0x00000000);
        c.line(16, 16, 26, 16, kWhite, 2);
        c.line(16, 16, 16, 26, kWhite, 2);
        c.line(48, 16, 38, 16, kWhite, 2);
        c.line(48, 16, 48, 26, kWhite, 2);
        c.line(16, 48, 26, 48, kWhite, 2);
        c.line(16, 48, 16, 38, kWhite, 2);
        c.line(48, 48, 38, 48, kWhite, 2);
        c.line(48, 48, 48, 38, kWhite, 2);
        iso_box(c, -0.35f, -0.25f, 0.4f, 0.7f, 0.55f, 0.6f, kTop, kLft, kRgt);
        break;
    }
    case Icon::Home:
        iso_box(c, -0.55f, -0.45f, 0.15f, 1.1f, 0.9f, 0.7f, kTop, kLft, kRgt);
        c.tri(16, 28, 32, 12, 48, 28, kRed);
        break;
    case Icon::Top: {
        c.quad(14, 20, 50, 20, 42, 44, 6, 44, kTop);
        c.line(14, 20, 50, 20, kInk, 1.4f);
        break;
    }
    case Icon::Front: {
        c.quad(12, 16, 52, 16, 52, 48, 12, 48, kGreen);
        c.line(12, 32, 52, 32, kInk, 1.2f);
        break;
    }
    case Icon::Right: {
        c.quad(20, 14, 50, 20, 50, 50, 20, 44, kRed);
        break;
    }
    case Icon::Shaded:
        iso_box(c, -0.65f, -0.5f, 0.2f, 1.3f, 1.05f, 1.1f, kTop, kLft, kRgt);
        break;
    case Icon::Wire: {
        float s[8][2];
        auto P = [&](int i, float x, float y, float z) { c.iso(x, y, z, s[i][0], s[i][1]); };
        P(0, -0.7f, -0.5f, 0.2f);
        P(1, 0.7f, -0.5f, 0.2f);
        P(2, 0.7f, 0.5f, 0.2f);
        P(3, -0.7f, 0.5f, 0.2f);
        P(4, -0.7f, -0.5f, 1.4f);
        P(5, 0.7f, -0.5f, 1.4f);
        P(6, 0.7f, 0.5f, 1.4f);
        P(7, -0.7f, 0.5f, 1.4f);
        int e[] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
        for (int i = 0; i < 24; i += 2) c.line(s[e[i]][0], s[e[i]][1], s[e[i + 1]][0], s[e[i + 1]][1], kTop, 1.5f);
        break;
    }
    case Icon::PlaneXY: {
        c.quad(14, 22, 50, 18, 54, 46, 18, 50, 0x4FA3F798);
        c.line(14, 22, 50, 18, kBlue, 1.5f);
        break;
    }
    case Icon::PlaneXZ: {
        c.quad(16, 14, 50, 20, 48, 52, 14, 46, 0xE24B3C98);
        break;
    }
    case Icon::PlaneYZ: {
        c.quad(22, 14, 48, 18, 44, 50, 18, 46, 0x6FCB6A98);
        break;
    }
    case Icon::Origin: {
        c.line(32, 32, 54, 24, kRed, 2.2f);
        c.line(32, 32, 14, 40, kGreen, 2.2f);
        c.line(32, 32, 32, 10, kBlue, 2.2f);
        c.disc(32, 32, 3, kWhite);
        break;
    }
    case Icon::Feature:
        iso_box(c, -0.5f, -0.4f, 0.3f, 1.0f, 0.8f, 0.9f, kTop, kLft, kRgt);
        break;
    case Icon::Sketch: {
        c.line(16, 44, 28, 18, kBlue, 1.8f);
        c.line(28, 18, 48, 40, kBlue, 1.8f);
        c.line(48, 40, 16, 44, kBlue, 1.8f);
        break;
    }
    case Icon::Component:
        iso_box(c, -0.8f, -0.35f, 0.15f, 0.85f, 0.7f, 0.7f, kTop, kLft, kRgt);
        iso_box(c, 0.05f, -0.2f, 0.55f, 0.75f, 0.6f, 0.65f, 0x8EC4F0FF, 0x3A6A9AFF, 0x5A8AB8FF);
        break;
    case Icon::Ground: {
        c.quad(12, 44, 52, 44, 52, 52, 12, 52, kSteel);
        iso_box(c, -0.45f, -0.35f, 0.45f, 0.9f, 0.7f, 0.7f, kTop, kLft, kRgt);
        break;
    }
    case Icon::Palette: {
        c.quad(16, 16, 48, 16, 48, 48, 16, 48, 0x2A3038FF);
        c.line(22, 24, 42, 24, kWhite, 1.4f);
        c.line(22, 32, 38, 32, kSteel, 1.4f);
        c.line(22, 40, 34, 40, kSteel, 1.4f);
        break;
    }
    case Icon::Help: {
        c.disc(32, 32, 20, kBlue);
        c.disc(32, 24, 3, kWhite);
        c.line(32, 30, 32, 40, kWhite, 2.4f);
        c.disc(32, 46, 2.5f, kWhite);
        break;
    }
    case Icon::Sweep: {
        c.line(12, 48, 20, 28, kBlue, 2.2f);
        c.line(20, 28, 44, 20, kBlue, 2.2f);
        c.line(44, 20, 52, 36, kBlue, 2.2f);
        iso_box(c, -0.25f, -0.25f, 0.9f, 0.5f, 0.5f, 0.5f, kTop, kLft, kRgt);
        break;
    }
    case Icon::Shell: {
        iso_box(c, -0.7f, -0.55f, 0.2f, 1.4f, 1.1f, 1.2f, kTop, kLft, kRgt);
        c.quad(24, 22, 40, 20, 42, 34, 26, 36, 0x2A3038FF);
        break;
    }
    case Icon::Coil: {
        for (int i = 0; i < 5; ++i) {
            float y = 46.f - i * 6.f;
            c.disc(32, y, 10.f - i * 0.4f, kRgt, false);
        }
        c.line(32, 50, 32, 12, kSteel, 1.6f);
        break;
    }
    case Icon::Slot: {
        c.quad(18, 26, 46, 26, 46, 40, 18, 40, kTop);
        c.disc(18, 33, 7, kTop);
        c.disc(46, 33, 7, kTop);
        c.disc(18, 33, 7, kInk, false);
        c.disc(46, 33, 7, kInk, false);
        break;
    }
    case Icon::Loft: {
        c.quad(16, 42, 36, 44, 38, 52, 14, 50, kLft);
        c.quad(26, 16, 48, 14, 50, 24, 28, 26, kTop);
        c.line(16, 42, 26, 16, kInk, 1.4f);
        c.line(36, 44, 48, 14, kInk, 1.4f);
        break;
    }
    case Icon::Undo: {
        c.line(20, 36, 44, 36, kBlue, 2.2f);
        c.line(20, 36, 28, 26, kBlue, 2.2f);
        c.line(20, 36, 28, 46, kBlue, 2.2f);
        break;
    }
    case Icon::Redo: {
        c.line(20, 36, 44, 36, kBlue, 2.2f);
        c.line(44, 36, 36, 26, kBlue, 2.2f);
        c.line(44, 36, 36, 46, kBlue, 2.2f);
        break;
    }
    case Icon::SketchArc: {
        for (int i = 0; i <= 14; ++i) {
            float t0 = kPi * i / 14.f, t1 = kPi * (i + 1) / 14.f;
            c.line(32 + std::cos(t0) * 18, 40 - std::sin(t0) * 18, 32 + std::cos(t1) * 18, 40 - std::sin(t1) * 18,
                   kBlue, 2.2f);
        }
        c.disc(14, 40, 3.5f, kWhite);
        c.disc(50, 40, 3.5f, kWhite);
        c.disc(32, 22, 3, kWhite);
        break;
    }
    case Icon::Gear: {
        for (int i = 0; i < 10; ++i) {
            float a0 = kTau * i / 10.f, a1 = kTau * (i + 0.45f) / 10.f;
            float r0 = 22.f, r1 = 16.f;
            c.quad(32 + std::cos(a0) * r1, 32 + std::sin(a0) * r1, 32 + std::cos(a0) * r0, 32 + std::sin(a0) * r0,
                   32 + std::cos(a1) * r0, 32 + std::sin(a1) * r0, 32 + std::cos(a1) * r1, 32 + std::sin(a1) * r1, kTop);
        }
        c.disc(32, 32, 8, 0x2A3038FF);
        c.disc(32, 32, 16, kInk, false);
        break;
    }
    case Icon::Thread: {
        for (int i = 0; i < 7; ++i) {
            float y = 14.f + i * 6.f;
            c.line(22, y, 42, y + 3, kRgt, 1.8f);
        }
        c.line(22, 12, 22, 54, kSteel, 1.6f);
        c.line(42, 12, 42, 54, kSteel, 1.6f);
        break;
    }
    case Icon::Rib: {
        iso_box(c, -0.95f, -0.55f, 0.15f, 0.35f, 1.1f, 1.25f, kTop, kLft, kRgt);
        float ax, ay, bx, by, cx, cy;
        c.iso(-0.6f, -0.1f, 0.15f, ax, ay);
        c.iso(0.9f, -0.1f, 0.15f, bx, by);
        c.iso(-0.6f, -0.1f, 1.3f, cx, cy);
        c.tri(ax, ay, bx, by, cx, cy, kTop);
        break;
    }
    case Icon::Drawing: {
        c.quad(12, 10, 52, 10, 52, 54, 12, 54, kWhite);
        c.line(12, 10, 52, 10, kInk, 1.2f);
        c.quad(16, 14, 30, 14, 30, 28, 16, 28, 0x00000000);
        c.line(16, 14, 30, 14, kBlue, 1.3f);
        c.line(30, 14, 30, 28, kBlue, 1.3f);
        c.line(30, 28, 16, 28, kBlue, 1.3f);
        c.line(16, 28, 16, 14, kBlue, 1.3f);
        iso_box(c, 0.15f, -0.15f, 0.35f, 0.7f, 0.55f, 0.55f, kTop, kLft, kRgt);
        c.quad(16, 44, 48, 44, 48, 50, 16, 50, kSteel);
        break;
    }
    case Icon::Interfere: {
        iso_box(c, -1.05f, -0.45f, 0.2f, 1.15f, 0.9f, 0.85f, kTop, kLft, kRgt);
        iso_box(c, -0.15f, -0.25f, 0.45f, 1.1f, 0.85f, 0.8f, 0xE24B3CCC, 0x8A2A22CC, 0xC04038CC);
        break;
    }
    case Icon::Com: {
        c.line(12, 32, 52, 32, kRed, 1.6f);
        c.line(32, 12, 32, 52, kGreen, 1.6f);
        c.disc(32, 32, 7, kTop);
        c.disc(32, 32, 3, kWhite);
        break;
    }
    case Icon::Copy: {
        iso_box(c, -0.95f, -0.55f, 0.15f, 0.95f, 0.8f, 0.85f, 0xC8C8D0FF, 0x888898FF, 0xA8A8B8FF);
        iso_box(c, -0.25f, -0.25f, 0.45f, 0.95f, 0.8f, 0.85f, kTop, kLft, kRgt);
        break;
    }
    case Icon::Isolate: {
        iso_box(c, -0.45f, -0.35f, 0.35f, 0.95f, 0.8f, 1.0f, kTop, kLft, kRgt);
        c.quad(10, 10, 22, 10, 22, 22, 10, 22, 0x00000000);
        c.line(10, 10, 22, 22, kRed, 1.8f);
        c.line(22, 10, 10, 22, kRed, 1.8f);
        break;
    }
    case Icon::Snap: {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j) c.disc(16.f + i * 11.f, 16.f + j * 11.f, 2.2f, kSteel);
        c.disc(38, 27, 4, kTop);
        break;
    }
    case Icon::Scale: {
        iso_box(c, -0.85f, -0.55f, 0.15f, 0.7f, 0.55f, 0.55f, kSteel, 0x5A6270FF, 0x7A8490FF);
        iso_box(c, -0.15f, -0.15f, 0.45f, 1.15f, 0.95f, 1.05f, kTop, kLft, kRgt);
        break;
    }
    case Icon::WorkPlane: {
        c.quad(10, 20, 50, 14, 54, 46, 14, 52, 0x4FA3F7AA);
        c.line(10, 20, 50, 14, kBlue, 1.6f);
        c.line(32, 8, 32, 28, kGreen, 2.0f);
        c.tri(32, 6, 27, 16, 37, 16, kGreen);
        break;
    }
    case Icon::Construction: {
        for (int i = 0; i < 8; ++i) {
            float t = i / 8.f;
            float x0 = 14 + t * 34, y0 = 46 - t * 30;
            float x1 = x0 + 4, y1 = y0 - 3.5f;
            c.line(x0, y0, x1, y1, kBlue, 1.5f);
        }
        c.disc(14, 46, 3, kWhite);
        c.disc(48, 16, 3, kWhite);
        break;
    }
    case Icon::Torus: {
        c.disc(32, 32, 20, kRgt);
        c.disc(32, 32, 12, 0x2A3038FF);
        c.disc(32, 32, 20, kInk, false);
        c.disc(32, 32, 12, kInk, false);
        break;
    }
    case Icon::Pipe: {
        c.line(12, 48, 28, 22, kSteel, 5.5f);
        c.line(28, 22, 50, 16, kTop, 5.5f);
        c.line(12, 48, 28, 22, kInk, 1.4f);
        c.line(28, 22, 50, 16, kInk, 1.4f);
        c.disc(12, 48, 4, kRgt);
        c.disc(50, 16, 4, kTop);
        break;
    }
    case Icon::Helix: {
        for (int i = 0; i < 18; ++i) {
            float t0 = i / 18.f, t1 = (i + 1) / 18.f;
            float x0 = 32 + std::cos(t0 * 5.2f) * (18 - t0 * 6);
            float y0 = 50 - t0 * 38;
            float x1 = 32 + std::cos(t1 * 5.2f) * (18 - t1 * 6);
            float y1 = 50 - t1 * 38;
            c.line(x0, y0, x1, y1, (i & 1) ? kTop : kRgt, 2.0f);
        }
        break;
    }
    case Icon::PathPattern: {
        c.line(12, 48, 28, 28, kBlue, 1.6f);
        c.line(28, 28, 50, 16, kBlue, 1.6f);
        iso_box(c, -0.95f, -0.2f, 0.05f, 0.45f, 0.4f, 0.4f, kTop, kLft, kRgt);
        iso_box(c, -0.15f, 0.15f, 0.45f, 0.45f, 0.4f, 0.4f, kTop, kLft, kRgt);
        iso_box(c, 0.55f, 0.45f, 0.85f, 0.45f, 0.4f, 0.4f, kTop, kLft, kRgt);
        break;
    }
    case Icon::Thicken: {
        iso_box(c, -0.55f, -0.4f, 0.25f, 1.1f, 0.85f, 0.7f, kSteel, 0x5A6270FF, 0x7A8490FF);
        iso_box(c, -0.75f, -0.55f, 0.05f, 1.5f, 1.15f, 1.15f, kTop, kLft, kRgt);
        break;
    }
    case Icon::Draft: {
        c.quad(18, 48, 46, 48, 40, 16, 24, 16, kRgt);
        c.quad(24, 16, 40, 16, 38, 12, 26, 12, kTop);
        c.line(32, 8, 32, 20, kGreen, 2.0f);
        c.tri(32, 6, 27, 15, 37, 15, kGreen);
        break;
    }
    case Icon::Ellipse: {
        for (int i = 0; i < 24; ++i) {
            float t0 = kTau * i / 24.f, t1 = kTau * (i + 1) / 24.f;
            c.line(32 + std::cos(t0) * 22, 32 + std::sin(t0) * 14, 32 + std::cos(t1) * 22, 32 + std::sin(t1) * 14,
                   kBlue, 1.8f);
        }
        c.disc(32, 32, 2.5f, kWhite);
        break;
    }
    case Icon::Spline: {
        c.disc(12, 46, 3, kWhite);
        c.disc(28, 18, 3, kBlue);
        c.disc(50, 36, 3, kWhite);
        for (int i = 0; i < 16; ++i) {
            float t0 = i / 16.f, t1 = (i + 1) / 16.f;
            auto bez = [](float t) {
                float u = 1.f - t;
                return Vec2{12 * u * u + 28 * 2 * u * t + 50 * t * t, 46 * u * u + 18 * 2 * u * t + 36 * t * t};
            };
            Vec2 a = bez(t0), b = bez(t1);
            c.line(a.x, a.y, b.x, b.y, kTop, 2.0f);
        }
        break;
    }
    case Icon::Params: {
        c.quad(14, 12, 50, 12, 50, 52, 14, 52, kWhite);
        c.line(18, 22, 46, 22, kBlue, 1.4f);
        c.line(18, 32, 40, 32, kBlue, 1.4f);
        c.line(18, 42, 36, 42, kBlue, 1.4f);
        break;
    }
    case Icon::Thickness: {
        iso_box(c, -0.7f, -0.5f, 0.2f, 1.4f, 1.05f, 0.35f, kTop, kLft, kRgt);
        c.line(22, 28, 22, 44, kGreen, 2.0f);
        c.line(42, 22, 42, 38, kGreen, 2.0f);
        break;
    }
    case Icon::Groove: {
        iso_box(c, -0.35f, -0.35f, 0.1f, 0.7f, 0.7f, 1.4f, kSteel, kLft, kRgt);
        c.line(20, 26, 44, 18, kRed, 2.2f);
        c.line(20, 38, 44, 30, kRed, 2.2f);
        break;
    }
    case Icon::Pocket: {
        iso_box(c, -0.85f, -0.7f, 0.05f, 1.7f, 1.4f, 0.35f, kTop, kLft, kRgt);
        c.quad(24, 28, 40, 28, 40, 40, 24, 40, kBlueDk);
        break;
    }
    case Icon::Boss: {
        iso_box(c, -0.8f, -0.7f, 0.0f, 1.6f, 1.4f, 0.25f, kTop, kLft, kRgt);
        c.disc(32, 26, 9, kSteel);
        c.disc(32, 26, 4, kBlueDk);
        break;
    }
    case Icon::Keyway: {
        c.disc(32, 32, 18, kSteel);
        c.disc(32, 32, 8, kBlueDk);
        c.quad(38, 26, 50, 26, 50, 38, 38, 38, kTop);
        break;
    }
    case Icon::Hex: {
        for (int i = 0; i < 6; ++i) {
            float a0 = (30.f + 60.f * i) * 0.0174533f, a1 = (30.f + 60.f * (i + 1)) * 0.0174533f;
            c.line(32 + std::cos(a0) * 20, 32 + std::sin(a0) * 20, 32 + std::cos(a1) * 20, 32 + std::sin(a1) * 20,
                   kTop, 2.0f);
        }
        c.disc(32, 32, 3, kWhite);
        break;
    }
    case Icon::Split: {
        iso_box(c, -0.7f, -0.55f, 0.1f, 1.4f, 1.1f, 1.0f, kTop, kLft, kRgt);
        c.line(14, 18, 50, 46, kRed, 2.2f);
        break;
    }
    case Icon::WorkAxis: {
        c.line(14, 50, 50, 14, kBlue, 2.0f);
        c.disc(14, 50, 3, kWhite);
        c.disc(50, 14, 3, kWhite);
        break;
    }
    case Icon::WorkPoint: {
        c.line(18, 32, 46, 32, kBlue, 1.6f);
        c.line(32, 18, 32, 46, kBlue, 1.6f);
        c.disc(32, 32, 4, kRed);
        break;
    }
    case Icon::Polygon: {
        for (int i = 0; i < 6; ++i) {
            float a0 = kTau * i / 6.f, a1 = kTau * (i + 1) / 6.f;
            c.line(32 + std::cos(a0) * 20, 32 + std::sin(a0) * 20, 32 + std::cos(a1) * 20, 32 + std::sin(a1) * 20,
                   kBlue, 1.8f);
        }
        break;
    }
    case Icon::Offset: {
        c.quad(18, 18, 46, 18, 46, 46, 18, 46, 0x00000000);
        c.line(18, 18, 46, 18, kBlue, 1.6f);
        c.line(46, 18, 46, 46, kBlue, 1.6f);
        c.line(46, 46, 18, 46, kBlue, 1.6f);
        c.line(18, 46, 18, 18, kBlue, 1.6f);
        c.line(24, 24, 40, 24, kTop, 1.6f);
        c.line(40, 24, 40, 40, kTop, 1.6f);
        c.line(40, 40, 24, 40, kTop, 1.6f);
        c.line(24, 40, 24, 24, kTop, 1.6f);
        break;
    }
    case Icon::Capture: {
        c.quad(14, 18, 50, 18, 50, 46, 14, 46, kWhite);
        c.disc(32, 32, 8, kBlue);
        c.quad(40, 16, 50, 16, 50, 22, 40, 22, kRed);
        break;
    }
    case Icon::Text: {
        c.line(16, 48, 16, 16, kTop, 2.2f);
        c.line(16, 16, 28, 16, kTop, 2.2f);
        c.line(36, 16, 48, 16, kBlue, 2.0f);
        c.line(42, 16, 42, 48, kBlue, 2.0f);
        c.line(36, 48, 48, 48, kBlue, 2.0f);
        break;
    }
    case Icon::Zebra: {
        iso_box(c, -0.7f, -0.55f, 0.1f, 1.4f, 1.1f, 1.0f, kTop, kLft, kRgt);
        c.line(20, 18, 28, 46, kInk, 2.4f);
        c.line(32, 16, 40, 46, kInk, 2.4f);
        c.line(44, 16, 50, 40, kInk, 2.4f);
        break;
    }
    case Icon::Dxf: {
        c.quad(14, 12, 50, 12, 50, 52, 14, 52, kWhite);
        c.line(20, 22, 44, 22, kInk, 1.6f);
        c.line(20, 32, 36, 32, kInk, 1.6f);
        c.line(20, 42, 40, 42, kBlue, 1.6f);
        break;
    }
    case Icon::Capsule: {
        c.disc(18, 32, 8, kSteel);
        c.disc(46, 32, 8, kSteel);
        c.quad(18, 24, 46, 24, 46, 40, 18, 40, kSteel);
        c.line(10, 32, 54, 32, kInk, 1.1f);
        break;
    }
    case Icon::Wedge: {
        c.tri(14, 48, 50, 48, 14, 16, kTop);
        c.line(14, 16, 50, 48, kInk, 1.8f);
        break;
    }
    case Icon::Angle: {
        c.quad(16, 16, 24, 16, 24, 48, 16, 48, kSteel);
        c.quad(16, 40, 50, 40, 50, 48, 16, 48, kTop);
        break;
    }
    case Icon::Channel: {
        c.quad(16, 16, 24, 16, 24, 48, 16, 48, kSteel);
        c.quad(16, 16, 48, 16, 48, 24, 16, 24, kTop);
        c.quad(16, 40, 48, 40, 48, 48, 16, 48, kTop);
        break;
    }
    case Icon::IBeam: {
        c.quad(18, 14, 46, 14, 46, 22, 18, 22, kTop);
        c.quad(28, 22, 36, 22, 36, 42, 28, 42, kSteel);
        c.quad(18, 42, 46, 42, 46, 50, 18, 50, kTop);
        break;
    }
    case Icon::Dovetail: {
        c.quad(22, 44, 42, 44, 50, 20, 14, 20, kTop);
        c.line(22, 44, 42, 44, kInk, 1.4f);
        break;
    }
    case Icon::Bolt: {
        c.quad(26, 12, 38, 12, 38, 22, 26, 22, kTop);
        c.quad(29, 22, 35, 22, 35, 50, 29, 50, kSteel);
        break;
    }
    case Icon::Nut: {
        for (int i = 0; i < 6; ++i) {
            float a0 = (30.f + 60.f * i) * 0.0174533f, a1 = (30.f + 60.f * (i + 1)) * 0.0174533f;
            c.line(32 + std::cos(a0) * 18, 32 + std::sin(a0) * 18, 32 + std::cos(a1) * 18, 32 + std::sin(a1) * 18,
                   kTop, 2.0f);
        }
        c.disc(32, 32, 6, kBlueDk);
        break;
    }
    case Icon::Washer: {
        c.disc(32, 32, 18, kSteel);
        c.disc(32, 32, 8, kBlueDk);
        break;
    }
    case Icon::Holes: {
        c.disc(22, 24, 7, kSteel);
        c.disc(22, 24, 3, kBlueDk);
        c.disc(42, 24, 7, kSteel);
        c.disc(42, 24, 3, kBlueDk);
        c.disc(32, 42, 7, kSteel);
        c.disc(32, 42, 3, kBlueDk);
        break;
    }
    case Icon::Print: {
        c.quad(12, 28, 52, 28, 52, 44, 12, 44, kSteel);
        c.quad(20, 16, 44, 16, 44, 28, 20, 28, kTop);
        c.quad(18, 44, 46, 44, 44, 52, 20, 52, kOrange);
        c.quad(40, 20, 50, 20, 50, 26, 40, 26, kBlue);
        break;
    }
    case Icon::Slice: {
        c.quad(14, 42, 50, 42, 46, 50, 18, 50, kSteel);
        c.quad(16, 32, 48, 32, 46, 40, 18, 40, kTop);
        c.quad(18, 22, 46, 22, 44, 30, 20, 30, kOrange);
        c.line(10, 18, 54, 18, kInk, 1.3f);
        break;
    }
    case Icon::Infill: {
        c.quad(14, 14, 50, 14, 50, 50, 14, 50, kBlueDk);
        for (int i = 0; i < 5; ++i) {
            float t = 18.f + i * 7.f;
            c.line(16, t, 48, t, kOrange, 1.3f);
            c.line(t, 16, t, 48, kTop, 1.1f);
        }
        break;
    }
    case Icon::Support: {
        c.quad(16, 14, 48, 14, 44, 22, 20, 22, kTop);
        c.line(22, 22, 22, 48, kSteel, 2.2f);
        c.line(32, 22, 32, 48, kSteel, 2.2f);
        c.line(42, 22, 42, 48, kSteel, 2.2f);
        c.quad(14, 48, 50, 48, 50, 52, 14, 52, kOrange);
        break;
    }
    case Icon::Gcode: {
        c.quad(16, 12, 40, 12, 40, 52, 16, 52, kTop);
        c.tri(40, 12, 48, 20, 40, 20, kTop);
        c.quad(40, 20, 48, 20, 48, 52, 40, 52, kTop);
        c.line(40, 12, 40, 20, kInk, 1.2f);
        c.line(40, 20, 48, 20, kInk, 1.2f);
        c.line(22, 28, 42, 28, kOrange, 1.6f);
        c.line(22, 36, 38, 36, kSteel, 1.4f);
        c.line(22, 44, 34, 44, kSteel, 1.4f);
        break;
    }
    default:
        break;
    }
}

bool IconAtlas::init() {
    cell = 64;
    cols = 8;
    rows = ((int)Icon::COUNT + cols - 1) / cols;
    tw = cols * cell;
    th = rows * cell;
    std::vector<std::uint8_t> buf((size_t)tw * th * 4, 0);
    Canvas c;
    c.buf = &buf;
    c.tw = tw;
    c.th = th;
    c.cell = cell;
    for (int i = 0; i < (int)Icon::COUNT; ++i) {
        c.ox = (i % cols) * cell;
        c.oy = (i / cols) * cell;
        // subtle plate so icons read on any chrome
        for (int y = 4; y < cell - 4; ++y)
            for (int x = 4; x < cell - 4; ++x) {
                float t = (float)y / (float)cell;
                std::uint8_t r = (std::uint8_t)(28 + t * 10);
                std::uint8_t g = (std::uint8_t)(30 + t * 10);
                std::uint8_t b = (std::uint8_t)(34 + t * 8);
                size_t p = ((size_t)(c.oy + y) * tw + (c.ox + x)) * 4;
                buf[p] = r;
                buf[p + 1] = g;
                buf[p + 2] = b;
                buf[p + 3] = 220;
            }
        paint(c, (Icon)i);
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex != 0;
}

void IconAtlas::shutdown() {
    if (tex) glDeleteTextures(1, &tex);
    tex = 0;
}

void IconAtlas::uv(Icon id, float uv0[2], float uv1[2]) const {
    int i = (int)id;
    int col = i % cols, row = i / cols;
    uv0[0] = (float)(col * cell) / (float)tw;
    uv0[1] = (float)(row * cell) / (float)th;
    uv1[0] = (float)((col + 1) * cell) / (float)tw;
    uv1[1] = (float)((row + 1) * cell) / (float)th;
}

Icon icon_for_feature(int kind) {
    switch ((FeatureKind)kind) {
    case FeatureKind::Box: return Icon::Box;
    case FeatureKind::Sphere: return Icon::Sphere;
    case FeatureKind::Cylinder: return Icon::Cylinder;
    case FeatureKind::Cone: return Icon::Cone;
    case FeatureKind::Sketch: return Icon::Sketch;
    case FeatureKind::Extrude: return Icon::Extrude;
    case FeatureKind::Revolve: return Icon::Revolve;
    case FeatureKind::Fillet: return Icon::Fillet;
    case FeatureKind::Chamfer: return Icon::Chamfer;
    case FeatureKind::Hole: return Icon::Hole;
    case FeatureKind::RectPattern: return Icon::RectPattern;
    case FeatureKind::CircPattern: return Icon::CircPattern;
    case FeatureKind::Mirror: return Icon::Mirror;
    case FeatureKind::Sweep: return Icon::Sweep;
    case FeatureKind::Shell: return Icon::Shell;
    case FeatureKind::Coil: return Icon::Coil;
    case FeatureKind::Slot: return Icon::Slot;
    case FeatureKind::Loft: return Icon::Loft;
    case FeatureKind::Rib: return Icon::Rib;
    case FeatureKind::Thread: return Icon::Thread;
    case FeatureKind::Gear: return Icon::Gear;
    case FeatureKind::WorkPlane: return Icon::WorkPlane;
    case FeatureKind::Torus: return Icon::Torus;
    case FeatureKind::Pipe: return Icon::Pipe;
    case FeatureKind::Helix: return Icon::Helix;
    case FeatureKind::PathPattern: return Icon::PathPattern;
    case FeatureKind::Thicken: return Icon::Thicken;
    case FeatureKind::Draft: return Icon::Draft;
    case FeatureKind::Groove: return Icon::Groove;
    case FeatureKind::Pocket: return Icon::Pocket;
    case FeatureKind::Boss: return Icon::Boss;
    case FeatureKind::Keyway: return Icon::Keyway;
    case FeatureKind::Hex: return Icon::Hex;
    case FeatureKind::Split: return Icon::Split;
    case FeatureKind::WorkAxis: return Icon::WorkAxis;
    case FeatureKind::WorkPoint: return Icon::WorkPoint;
    case FeatureKind::Text: return Icon::Text;
    case FeatureKind::Capsule: return Icon::Capsule;
    case FeatureKind::Wedge: return Icon::Wedge;
    case FeatureKind::Angle: return Icon::Angle;
    case FeatureKind::Channel: return Icon::Channel;
    case FeatureKind::IBeam: return Icon::IBeam;
    case FeatureKind::Dovetail: return Icon::Dovetail;
    case FeatureKind::Bolt: return Icon::Bolt;
    case FeatureKind::Nut: return Icon::Nut;
    case FeatureKind::Washer: return Icon::Washer;
    case FeatureKind::Import: return Icon::Open;
    }
    return Icon::Feature;
}

Icon icon_for_tool(int tool) {
    switch ((Tool)tool) {
    case Tool::Select: return Icon::Select;
    case Tool::Box: return Icon::Box;
    case Tool::Cylinder: return Icon::Cylinder;
    case Tool::Sphere: return Icon::Sphere;
    case Tool::Cone: return Icon::Cone;
    case Tool::SketchRect: return Icon::SketchRect;
    case Tool::SketchCircle: return Icon::SketchCircle;
    case Tool::SketchLine: return Icon::SketchLine;
    case Tool::Extrude: return Icon::Extrude;
    case Tool::Revolve: return Icon::Revolve;
    case Tool::Fillet: return Icon::Fillet;
    case Tool::Chamfer: return Icon::Chamfer;
    case Tool::Hole: return Icon::Hole;
    case Tool::RectPattern: return Icon::RectPattern;
    case Tool::CircPattern: return Icon::CircPattern;
    case Tool::Mirror: return Icon::Mirror;
    case Tool::Measure: return Icon::Measure;
    case Tool::Sweep: return Icon::Sweep;
    case Tool::Shell: return Icon::Shell;
    case Tool::Coil: return Icon::Coil;
    case Tool::Slot: return Icon::Slot;
    case Tool::Loft: return Icon::Loft;
    case Tool::SketchArc: return Icon::SketchArc;
    case Tool::Gear: return Icon::Gear;
    case Tool::Thread: return Icon::Thread;
    case Tool::Rib: return Icon::Rib;
    case Tool::SketchFillet: return Icon::Fillet;
    case Tool::WorkPlane: return Icon::WorkPlane;
    case Tool::Scale: return Icon::Scale;
    case Tool::Move: return Icon::Select;
    case Tool::Copy: return Icon::Copy;
    case Tool::Torus: return Icon::Torus;
    case Tool::Pipe: return Icon::Pipe;
    case Tool::Helix: return Icon::Helix;
    case Tool::PathPattern: return Icon::PathPattern;
    case Tool::Thicken: return Icon::Thicken;
    case Tool::Draft: return Icon::Draft;
    case Tool::SketchEllipse: return Icon::Ellipse;
    case Tool::SketchSpline: return Icon::Spline;
    case Tool::Groove: return Icon::Groove;
    case Tool::Pocket: return Icon::Pocket;
    case Tool::Boss: return Icon::Boss;
    case Tool::Keyway: return Icon::Keyway;
    case Tool::Hex: return Icon::Hex;
    case Tool::Split: return Icon::Split;
    case Tool::WorkAxis: return Icon::WorkAxis;
    case Tool::WorkPoint: return Icon::WorkPoint;
    case Tool::SketchPolygon: return Icon::Polygon;
    case Tool::SketchOffset: return Icon::Offset;
    case Tool::Text: return Icon::Text;
    case Tool::SketchMirror: return Icon::Mirror;
    case Tool::Capsule: return Icon::Capsule;
    case Tool::Wedge: return Icon::Wedge;
    case Tool::Angle: return Icon::Angle;
    case Tool::Channel: return Icon::Channel;
    case Tool::IBeam: return Icon::IBeam;
    case Tool::Dovetail: return Icon::Dovetail;
    case Tool::Bolt: return Icon::Bolt;
    case Tool::Nut: return Icon::Nut;
    case Tool::Washer: return Icon::Washer;
    }
    return Icon::Select;
}

} // namespace ax
