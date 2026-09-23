#pragma once

#include "math.hpp"
#include <cctype>
#include <vector>

namespace ax {

// Stick font in a 0..1 × 0..1 em square. Each glyph is a list of strokes (pairs).
inline void stroke_glyph(char ch, std::vector<Vec2>& segs) {
    auto s = [&](float x0, float y0, float x1, float y1) {
        segs.push_back({x0, y0});
        segs.push_back({x1, y1});
    };
    ch = (char)std::toupper((unsigned char)ch);
    switch (ch) {
    case 'A':
        s(0, 0, 0.5f, 1); s(0.5f, 1, 1, 0); s(0.18f, 0.38f, 0.82f, 0.38f);
        break;
    case 'B':
        s(0, 0, 0, 1); s(0, 1, 0.7f, 1); s(0.7f, 1, 0.85f, 0.78f); s(0.85f, 0.78f, 0.7f, 0.52f);
        s(0.7f, 0.52f, 0, 0.52f); s(0.7f, 0.52f, 0.9f, 0.28f); s(0.9f, 0.28f, 0.7f, 0); s(0.7f, 0, 0, 0);
        break;
    case 'C':
        s(0.92f, 0.82f, 0.55f, 1); s(0.55f, 1, 0.12f, 0.78f); s(0.12f, 0.78f, 0.12f, 0.22f);
        s(0.12f, 0.22f, 0.55f, 0); s(0.55f, 0, 0.92f, 0.18f);
        break;
    case 'D':
        s(0, 0, 0, 1); s(0, 1, 0.62f, 1); s(0.62f, 1, 0.95f, 0.7f); s(0.95f, 0.7f, 0.95f, 0.3f);
        s(0.95f, 0.3f, 0.62f, 0); s(0.62f, 0, 0, 0);
        break;
    case 'E':
        s(0.88f, 1, 0, 1); s(0, 1, 0, 0); s(0, 0, 0.88f, 0); s(0, 0.5f, 0.7f, 0.5f);
        break;
    case 'F':
        s(0, 0, 0, 1); s(0, 1, 0.88f, 1); s(0, 0.5f, 0.68f, 0.5f);
        break;
    case 'G':
        s(0.92f, 0.82f, 0.55f, 1); s(0.55f, 1, 0.12f, 0.78f); s(0.12f, 0.78f, 0.12f, 0.22f);
        s(0.12f, 0.22f, 0.55f, 0); s(0.55f, 0, 0.92f, 0.22f); s(0.92f, 0.22f, 0.92f, 0.48f);
        s(0.55f, 0.48f, 0.92f, 0.48f);
        break;
    case 'H':
        s(0, 0, 0, 1); s(1, 0, 1, 1); s(0, 0.5f, 1, 0.5f);
        break;
    case 'I':
        s(0.2f, 1, 0.8f, 1); s(0.5f, 1, 0.5f, 0); s(0.2f, 0, 0.8f, 0);
        break;
    case 'J':
        s(0.15f, 1, 0.95f, 1); s(0.7f, 1, 0.7f, 0.22f); s(0.7f, 0.22f, 0.4f, 0); s(0.4f, 0, 0.08f, 0.18f);
        break;
    case 'K':
        s(0, 0, 0, 1); s(0.92f, 1, 0, 0.48f); s(0.22f, 0.58f, 0.95f, 0);
        break;
    case 'L':
        s(0, 1, 0, 0); s(0, 0, 0.9f, 0);
        break;
    case 'M':
        s(0, 0, 0, 1); s(0, 1, 0.5f, 0.4f); s(0.5f, 0.4f, 1, 1); s(1, 1, 1, 0);
        break;
    case 'N':
        s(0, 0, 0, 1); s(0, 1, 1, 0); s(1, 0, 1, 1);
        break;
    case 'O':
        s(0.2f, 0, 0.8f, 0); s(0.8f, 0, 1, 0.22f); s(1, 0.22f, 1, 0.78f); s(1, 0.78f, 0.8f, 1);
        s(0.8f, 1, 0.2f, 1); s(0.2f, 1, 0, 0.78f); s(0, 0.78f, 0, 0.22f); s(0, 0.22f, 0.2f, 0);
        break;
    case 'P':
        s(0, 0, 0, 1); s(0, 1, 0.75f, 1); s(0.75f, 1, 0.95f, 0.78f); s(0.95f, 0.78f, 0.75f, 0.52f);
        s(0.75f, 0.52f, 0, 0.52f);
        break;
    case 'Q':
        s(0.2f, 0, 0.8f, 0); s(0.8f, 0, 1, 0.22f); s(1, 0.22f, 1, 0.78f); s(1, 0.78f, 0.8f, 1);
        s(0.8f, 1, 0.2f, 1); s(0.2f, 1, 0, 0.78f); s(0, 0.78f, 0, 0.22f); s(0, 0.22f, 0.2f, 0);
        s(0.55f, 0.28f, 1, 0);
        break;
    case 'R':
        s(0, 0, 0, 1); s(0, 1, 0.72f, 1); s(0.72f, 1, 0.92f, 0.78f); s(0.92f, 0.78f, 0.72f, 0.5f);
        s(0.72f, 0.5f, 0, 0.5f); s(0.4f, 0.5f, 0.95f, 0);
        break;
    case 'S':
        s(0.9f, 0.82f, 0.55f, 1); s(0.55f, 1, 0.12f, 0.82f); s(0.12f, 0.82f, 0.18f, 0.55f);
        s(0.18f, 0.55f, 0.82f, 0.42f); s(0.82f, 0.42f, 0.88f, 0.16f); s(0.88f, 0.16f, 0.5f, 0);
        s(0.5f, 0, 0.08f, 0.16f);
        break;
    case 'T':
        s(0, 1, 1, 1); s(0.5f, 1, 0.5f, 0);
        break;
    case 'U':
        s(0, 1, 0, 0.22f); s(0, 0.22f, 0.22f, 0); s(0.22f, 0, 0.78f, 0); s(0.78f, 0, 1, 0.22f); s(1, 0.22f, 1, 1);
        break;
    case 'V':
        s(0, 1, 0.5f, 0); s(0.5f, 0, 1, 1);
        break;
    case 'W':
        s(0, 1, 0.22f, 0); s(0.22f, 0, 0.5f, 0.45f); s(0.5f, 0.45f, 0.78f, 0); s(0.78f, 0, 1, 1);
        break;
    case 'X':
        s(0, 1, 1, 0); s(1, 1, 0, 0);
        break;
    case 'Y':
        s(0, 1, 0.5f, 0.5f); s(1, 1, 0.5f, 0.5f); s(0.5f, 0.5f, 0.5f, 0);
        break;
    case 'Z':
        s(0, 1, 1, 1); s(1, 1, 0, 0); s(0, 0, 1, 0);
        break;
    case '0':
        s(0.2f, 0, 0.8f, 0); s(0.8f, 0, 1, 0.22f); s(1, 0.22f, 1, 0.78f); s(1, 0.78f, 0.8f, 1);
        s(0.8f, 1, 0.2f, 1); s(0.2f, 1, 0, 0.78f); s(0, 0.78f, 0, 0.22f); s(0, 0.22f, 0.2f, 0);
        s(0.15f, 0.18f, 0.85f, 0.82f);
        break;
    case '1':
        s(0.25f, 0.75f, 0.5f, 1); s(0.5f, 1, 0.5f, 0); s(0.22f, 0, 0.78f, 0);
        break;
    case '2':
        s(0.08f, 0.78f, 0.35f, 1); s(0.35f, 1, 0.78f, 1); s(0.78f, 1, 0.95f, 0.75f);
        s(0.95f, 0.75f, 0.12f, 0); s(0.12f, 0, 0.95f, 0);
        break;
    case '3':
        s(0.12f, 1, 0.82f, 1); s(0.82f, 1, 0.5f, 0.55f); s(0.5f, 0.55f, 0.85f, 0.5f);
        s(0.85f, 0.5f, 0.85f, 0.15f); s(0.85f, 0.15f, 0.45f, 0); s(0.45f, 0, 0.1f, 0.18f);
        break;
    case '4':
        s(0.72f, 0, 0.72f, 1); s(0.72f, 1, 0.08f, 0.38f); s(0.08f, 0.38f, 0.95f, 0.38f);
        break;
    case '5':
        s(0.88f, 1, 0.12f, 1); s(0.12f, 1, 0.12f, 0.55f); s(0.12f, 0.55f, 0.75f, 0.55f);
        s(0.75f, 0.55f, 0.92f, 0.3f); s(0.92f, 0.3f, 0.7f, 0); s(0.7f, 0, 0.15f, 0.12f);
        break;
    case '6':
        s(0.82f, 1, 0.28f, 0.85f); s(0.28f, 0.85f, 0.1f, 0.4f); s(0.1f, 0.4f, 0.22f, 0);
        s(0.22f, 0, 0.78f, 0); s(0.78f, 0, 0.92f, 0.28f); s(0.92f, 0.28f, 0.7f, 0.5f);
        s(0.7f, 0.5f, 0.15f, 0.42f);
        break;
    case '7':
        s(0.08f, 1, 0.95f, 1); s(0.95f, 1, 0.32f, 0);
        break;
    case '8':
        s(0.5f, 0.52f, 0.15f, 0.7f); s(0.15f, 0.7f, 0.22f, 1); s(0.22f, 1, 0.78f, 1);
        s(0.78f, 1, 0.85f, 0.7f); s(0.85f, 0.7f, 0.5f, 0.52f); s(0.5f, 0.52f, 0.15f, 0.3f);
        s(0.15f, 0.3f, 0.22f, 0); s(0.22f, 0, 0.78f, 0); s(0.78f, 0, 0.85f, 0.3f); s(0.85f, 0.3f, 0.5f, 0.52f);
        break;
    case '9':
        s(0.18f, 0, 0.72f, 0.15f); s(0.72f, 0.15f, 0.9f, 0.6f); s(0.9f, 0.6f, 0.78f, 1);
        s(0.78f, 1, 0.22f, 1); s(0.22f, 1, 0.08f, 0.72f); s(0.08f, 0.72f, 0.3f, 0.5f); s(0.3f, 0.5f, 0.88f, 0.58f);
        break;
    case '-':
        s(0.1f, 0.5f, 0.9f, 0.5f);
        break;
    case '+':
        s(0.1f, 0.5f, 0.9f, 0.5f); s(0.5f, 0.15f, 0.5f, 0.85f);
        break;
    case '.':
        s(0.4f, 0.05f, 0.55f, 0.05f); s(0.55f, 0.05f, 0.55f, 0.18f); s(0.55f, 0.18f, 0.4f, 0.18f);
        s(0.4f, 0.18f, 0.4f, 0.05f);
        break;
    case '/':
        s(0.1f, 0, 0.9f, 1);
        break;
    case ':':
        s(0.42f, 0.25f, 0.58f, 0.25f); s(0.42f, 0.75f, 0.58f, 0.75f);
        break;
    default:
        break;
    }
}

inline float stroke_text(const char* s, float height, float tracking, std::vector<Vec2>& segs) {
    if (!s || height < 0.2f) return 0;
    float x = 0;
    float adv = height * 0.78f;
    float gap = height * (tracking > 0.01f ? tracking : 0.18f);
    for (const char* p = s; *p; ++p) {
        if (*p == ' ') {
            x += adv * 0.55f;
            continue;
        }
        size_t n0 = segs.size();
        stroke_glyph(*p, segs);
        for (size_t i = n0; i < segs.size(); ++i) {
            segs[i].x = segs[i].x * (adv * 0.92f) + x;
            segs[i].y = segs[i].y * height;
        }
        x += adv + gap;
    }
    return x;
}

} // namespace ax
