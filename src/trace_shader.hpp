#pragma once

// GPU sphere-trace of the CSG SDF + image-space CAD contours.
// The viewport no longer depends on the export triangle mesh.

static const char* kTraceVS = R"(
#version 330 core
void main(){
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

static const char* kTraceFS = R"(
#version 330 core
uniform samplerBuffer uPrims;
uniform samplerBuffer uCurves;
uniform int uN;
uniform vec3 uEye;
uniform mat4 uInvVP;
uniform mat4 uVP;
uniform vec2 uRes;
uniform vec3 uColor;
uniform uint uSel;
uniform uint uHover;
uniform int uSection;
uniform vec4 uClip;
uniform float uFar;
uniform int uWire;
uniform int uContours;
uniform vec3 uBMin;
uniform vec3 uBMax;
out vec4 frag;

const int STRIDE = 16;
const float PI = 3.14159265;
const float TAU = 6.2831853;

vec4 texelP(int i){ return texelFetch(uPrims, i); }
vec3 curve(int i){ return texelFetch(uCurves, i).xyz; }

float sdBox(vec3 p, vec3 h){
    vec3 q = abs(p) - h;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}
float sdSphere(vec3 p, float r){ return length(p) - r; }
float sdCyl(vec3 p, float r, float h){
    float dxy = length(p.xy) - r;
    float dz = abs(p.z - h * 0.5) - h * 0.5;
    if(dxy > 0.0 && dz > 0.0) return length(vec2(dxy, dz));
    return max(dxy, dz);
}
float sdCone(vec3 p, float r, float h){
    if(h < 1e-4) return length(p);
    float q = length(p.xy);
    float t = clamp((r * (r - q) + h * p.z) / (r * r + h * h), 0.0, 1.0);
    vec2 closest = vec2(r * (1.0 - t), h * t);
    float dist = length(vec2(q, p.z) - closest);
    float inside = (q <= r * (1.0 - p.z / h) && p.z >= 0.0 && p.z <= h) ? -1.0 : 1.0;
    float dcap = (p.z < 0.0) ? length(vec2(max(q - r, 0.0), -p.z)) : 1e9;
    return min(dist * inside, dcap);
}
float sdRoundBox(vec3 p, vec3 h, float rad){
    rad = clamp(rad, 0.0, max(0.0, min(h.x, min(h.y, h.z)) - 0.05));
    return sdBox(p, h - vec3(rad)) - rad;
}
float sdExtrude2(float d, float z, float h){
    float wz = abs(z - h * 0.5) - h * 0.5;
    vec2 w = vec2(d, wz);
    return min(max(w.x, w.y), 0.0) + length(max(w, 0.0));
}
float smin(float a, float b, float k){
    if(k <= 1e-5) return min(a, b);
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}
float smax(float a, float b, float k){ return -smin(-a, -b, k); }

float sdPoly(vec2 p, int off, int n){
    if(n < 3) return 1e9;
    vec2 a0 = curve(off).xy;
    float d = dot(p - a0, p - a0);
    float s = 1.0;
    for(int i = 0; i < 96; ++i){
        if(i >= n) break;
        int j = (i + n - 1);
        if(j >= n) j -= n;
        vec2 a = curve(off + j).xy;
        vec2 b = curve(off + i).xy;
        vec2 e = b - a;
        vec2 w = p - a;
        float t = clamp(dot(e, w) / (dot(e, e) + 1e-12), 0.0, 1.0);
        vec2 b_ = a + e * t - p;
        d = min(d, dot(b_, b_));
        bool cond = (p.y >= a.y && p.y < b.y && e.x * w.y > e.y * w.x) ||
                    (p.y < a.y && p.y >= b.y && e.x * w.y < e.y * w.x);
        if(cond) s = -s;
    }
    return s * sqrt(d);
}

float sdExtrude(vec3 p, int off, int n, float h, float taper, bool mid){
    h = max(1e-3, h);
    float z0 = mid ? -h * 0.5 : 0.0;
    float midz = z0 + h * 0.5;
    float wz = abs(p.z - midz) - h * 0.5;
    float t = clamp((p.z - z0) / h, 0.0, 1.0);
    float offset = t * h * tan(taper * 0.017453292);
    float d = sdPoly(p.xy, off, n) + offset;
    vec2 w = vec2(d, wz);
    return min(max(w.x, w.y), 0.0) + length(max(w, 0.0));
}

float sdSweep(vec3 p, int poff, int pn, float radius, int foff, int fn){
    if(pn < 2) return 1e9;
    float best = 1e9;
    for(int i = 0; i < 64; ++i){
        if(i + 1 >= pn) break;
        vec3 a = curve(poff + i);
        vec3 b = curve(poff + i + 1);
        vec3 ab = b - a;
        float len2 = dot(ab, ab);
        if(len2 < 1e-10) continue;
        float t = clamp(dot(ab, p - a) / len2, 0.0, 1.0);
        vec3 c = a + ab * t;
        if(fn >= 3){
            vec3 T = ab * inversesqrt(len2);
            vec3 ref = abs(T.z) < 0.9 ? vec3(0,0,1) : vec3(1,0,0);
            vec3 N = normalize(cross(T, ref));
            vec3 B = cross(T, N);
            vec3 d = p - c;
            best = min(best, sdPoly(vec2(dot(d,N), dot(d,B)), foff, fn));
        } else {
            best = min(best, length(p - c) - radius);
        }
    }
    return best;
}

float sdCoil(vec3 p, float major, float minor, float pitch, float turns){
    major = max(0.2, major); minor = max(0.15, minor);
    pitch = max(0.4, pitch); turns = max(0.25, turns);
    float tau = atan(p.y, p.x);
    float frac = tau / TAU;
    if(frac < 0.0) frac += 1.0;
    float zturn = p.z / pitch;
    float k = floor(zturn - frac + 0.5);
    float t = clamp(frac + k, 0.0, turns);
    float a = t * TAU;
    vec3 h = vec3(major * cos(a), major * sin(a), t * pitch);
    return length(p - h) - minor;
}

float sdGear(vec3 p, float r_tip, float r_bore, int teeth, float h){
    teeth = max(teeth, 6);
    r_tip = max(1.0, r_tip);
    h = max(0.4, h);
    float module = (2.0 * r_tip) / float(teeth + 2);
    float r_pitch = 0.5 * module * float(teeth);
    float r_root = max(r_tip * 0.42, r_pitch - 1.25 * module);
    float r_base = max(0.4, r_pitch * cos(20.0 * 0.017453292));
    r_bore = clamp(r_bore, 0.0, r_root - 0.35);
    float ra = length(p.xy);
    float an = atan(p.y, p.x);
    float fa = TAU / float(teeth);
    float a = an - fa * floor(an / fa + 0.5);
    float involute = 0.0;
    {
        float rr = max(max(ra, r_base), r_base + 1e-3);
        float c = clamp(r_base / rr, 0.0, 1.0);
        float al = acos(c);
        involute = tan(al) - al;
    }
    float half_p = (PI / float(teeth)) * 0.5;
    float inv_p;
    {
        float rr = max(r_pitch, r_base + 1e-3);
        float c = clamp(r_base / rr, 0.0, 1.0);
        float al = acos(c);
        inv_p = tan(al) - al;
    }
    float hw = half_p + inv_p - involute;
    if(ra < r_root){
        float u = clamp((r_root - ra) / max(0.25, r_root * 0.35), 0.0, 1.0);
        float inv_r;
        float rr = max(r_root, r_base + 1e-3);
        float c = clamp(r_base / rr, 0.0, 1.0);
        float al = acos(c);
        inv_r = tan(al) - al;
        hw = mix(half_p + inv_p - inv_r, fa * 0.48, u);
    }
    hw = max(0.03 * fa, hw);
    float d_side = (abs(a) - hw) * max(ra, r_root);
    float disk = ra - r_root;
    float tooth = max(ra - r_tip, d_side);
    float d2 = smin(disk, tooth, module * 0.16);
    d2 = max(d2, r_bore - ra);
    return sdExtrude2(d2, p.z, h);
}

float sdThread(vec3 p, float major, float pitch, float len, float depth, bool internal){
    major = max(0.6, major); pitch = max(0.3, pitch); len = max(1.0, len);
    depth = depth > 0.08 ? depth : pitch * 0.54;
    float turns = max(1.0, len / pitch);
    if(internal){
        float hole = sdCyl(p, max(0.4, major - depth * 0.2), len);
        float groove = sdCoil(p, major - depth * 0.4, depth * 0.55, pitch, turns);
        return min(hole, groove);
    }
    float core = sdCyl(p, max(0.35, major - depth), len);
    float crest = sdCoil(p, major - depth * 0.5, depth * 0.55, pitch, turns);
    return min(core, crest);
}

float sdRib(vec3 p, float len, float height, float thickness){
    vec2 q = p.xz;
    vec2 a = vec2(0,0), b = vec2(len,0), c = vec2(0,height);
    float d = 1e9; float s = 1.0;
    vec2 pts0 = a, pts1 = b, pts2 = c;
    for(int i=0;i<3;++i){
        vec2 e0 = i==0?pts0:(i==1?pts1:pts2);
        vec2 e1 = i==0?pts1:(i==1?pts2:pts0);
        vec2 e = e1 - e0, w = q - e0;
        float t = clamp(dot(e,w)/(dot(e,e)+1e-12),0.0,1.0);
        d = min(d, length(e0 + e*t - q));
        if((q.y>=e0.y && q.y<e1.y && e.x*w.y>e.y*w.x) || (q.y<e0.y && q.y>=e1.y && e.x*w.y<e.y*w.x)) s = -s;
    }
    float d2 = s * d;
    float wy = abs(p.y) - thickness * 0.5;
    vec2 ww = vec2(d2, wy);
    return min(max(ww.x, ww.y), 0.0) + length(max(ww, 0.0));
}

float sdTorus(vec3 p, float R, float r, float sweep){
    R = max(0.4, R); r = max(0.12, r);
    if(sweep >= 359.0){
        vec2 q = vec2(length(p.xy) - R, p.z);
        return length(q) - r;
    }
    float a = atan(p.y, p.x);
    if(a < 0.0) a += TAU;
    float lim = clamp(sweep, 1.0, 360.0) * 0.017453292;
    float ac = clamp(a, 0.0, lim);
    vec3 c = vec3(R * cos(ac), R * sin(ac), 0.0);
    return length(p - c) - r;
}

float sdCapsule(vec3 p, float r, float h){
    r = max(0.15, r); h = max(2.0 * r, h);
    vec3 a = vec3(0.0, 0.0, r);
    vec3 b = vec3(0.0, 0.0, h - r);
    vec3 ba = b - a;
    float t = clamp(dot(p - a, ba) / (dot(ba, ba) + 1e-12), 0.0, 1.0);
    return length(p - (a + ba * t)) - r;
}
float sdWedge(vec3 p, vec3 s, int flip){
    s = max(s, vec3(0.2));
    if(flip != 0) p.x = s.x - p.x;
    vec3 h = s * 0.5;
    float db = sdBox(p - h, h);
    float sl = (s.z * p.x + s.x * (p.z - s.z)) / max(1e-6, length(vec2(s.z, s.x)));
    return max(db, -sl);
}
float sdHex(vec3 p, float r, float h){
    r = max(0.2, r); h = max(0.2, h);
    const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
    p.z -= h * 0.5;
    vec3 q = vec3(abs(p.x), abs(p.y), abs(p.z));
    q.xy -= 2.0 * min(dot(k.xy, q.xy), 0.0) * k.xy;
    vec2 d = vec2(length(q.xy - vec2(clamp(q.x, -k.z*r, k.z*r), r)) * sign(q.y - r), q.z - h*0.5);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float sdSlot(vec3 p, float L, float W, float H){
    float w = max(0.2, W);
    float hlf = max(0.0, (L - w) * 0.5);
    vec2 q = p.xy;
    vec2 a = vec2(-hlf, 0), b = vec2(hlf, 0);
    vec2 e = b - a;
    float t = clamp(dot(e, q - a) / (dot(e,e)+1e-12), 0.0, 1.0);
    float d = length(q - (a + e * t)) - w * 0.5;
    return sdExtrude2(d, p.z, max(0.2, H));
}

float edgeBlend(float d, vec3 p, vec3 a, vec3 b, vec3 n1, vec3 n2, float r, float chamfer){
    if(r < 0.02) return d;
    vec3 ab = b - a;
    float L2 = dot(ab, ab);
    if(L2 < 1e-8) return d;
    float t = clamp(dot(p - a, ab) / L2, 0.0, 1.0);
    vec3 q = a + ab * t;
    float rad = length(p - q);
    if(rad > r * 3.6) return d;
    n1 = length(n1) > 1e-6 ? normalize(n1) : vec3(0,0,1);
    n2 = length(n2) > 1e-6 ? normalize(n2) : vec3(0,1,0);
    if(abs(dot(n2, n1)) > 0.98){
        n2 = cross(normalize(ab), n1);
        if(dot(n2,n2) < 1e-8) n2 = vec3(0,1,0);
        n2 = normalize(n2);
    }
    float d1 = dot(p - a, n1);
    float d2 = dot(p - a, n2);
    if(d1 > r*2.4 || d2 > r*2.4 || d1 < -r*2.6 || d2 < -r*2.6) return d;
    float L = sqrt(L2);
    float endd = min(t * L, L - t * L);
    if(endd < -0.15 * r) return d;
    vec3 outw = n1 + n2;
    if(dot(outw,outw) < 1e-10) outw = n1;
    outw = normalize(outw);
    float nd = clamp(dot(n1, n2), -0.999, 0.999);
    float off = r / max(0.18, sin(0.5 * acos(nd)));
    float dconv = max(d1, d2);
    float dconc = min(d1, d2);
    bool convex = abs(d - dconv) <= abs(d - dconc) + 0.04;
    vec3 c0 = convex ? a - outw * off : a + outw * off;
    vec3 c1 = convex ? b - outw * off : b + outw * off;
    vec3 e = c1 - c0;
    float e2 = dot(e,e);
    float u = e2 > 1e-12 ? clamp(dot(p - c0, e) / e2, 0.0, 1.0) : 0.0;
    float dc = length(p - (c0 + e * u)) - r;
    float fade = clamp(endd / max(0.12, r * 0.55), 0.0, 1.0);
    fade *= clamp((r * 3.4 - rad) / max(0.08, r), 0.0, 1.0);
    float d2b;
    if(convex){
        float cut = chamfer > 0.5 ? (d1 + d2) * 0.70710678 - r : dc;
        d2b = max(d, cut);
    } else {
        float add = chamfer > 0.5 ? (d1 + d2) * 0.70710678 + r : dc;
        d2b = min(d, add);
    }
    return mix(d, d2b, fade);
}

float combine(int op, float a, float b, float k, float chamfer){
    if(op == 0) return b;
    if(chamfer > 0.5 && k > 1e-4){
        if(op == 1) return min(min(a,b), (a+b)*0.70710678 - k);
        if(op == 2) return max(max(a,-b), (a-b)*0.70710678 - k);
        return max(max(a,b), (a+b)*0.70710678 + k);
    }
    if(k > 1e-4){
        if(op == 1) return smin(a,b,k);
        if(op == 2) return smax(a,-b,k);
        return smax(a,b,k);
    }
    if(op == 1) return min(a,b);
    if(op == 2) return max(a,-b);
    return max(a,b);
}

struct HitInfo { float d; float fid; };

HitInfo mapScene(vec3 p){
    float d = 1e9;
    float fid = 0.0;
    bool have = false;
    for(int i = 0; i < 48; ++i){
        if(i >= uN) break;
        int b = i * STRIDE;
        int kind = int(texelP(b+0).x + 0.5);
        if(kind != 17) continue;
        vec4 a1 = texelP(b+1);
        vec4 a2 = texelP(b+2);
        float ang = a1.x * 0.017453292;
        float t = tan(ang);
        vec3 o = a2.xyz;
        float refh = max(8.0, a1.y);
        float s = max(0.25, 1.0 + t * (p.z - o.z) / refh);
        p.x = o.x + (p.x - o.x) / s;
        p.y = o.y + (p.y - o.y) / s;
    }
    for(int i = 0; i < 48; ++i){
        if(i >= uN) break;
        int b = i * STRIDE;
        vec4 a0 = texelP(b+0);
        vec4 a1 = texelP(b+1);
        vec4 a2 = texelP(b+2);
        vec4 a4 = texelP(b+4);
        vec4 a7 = texelP(b+7);
        vec4 a8 = texelP(b+8);
        int kind = int(a0.x + 0.5);
        int op = int(a0.y + 0.5);
        float blend = a0.z;
        float cham = a0.w;
        vec3 mn = a7.xyz, mx = a8.xyz;
        if(have && op != 0 && blend < 0.02){
            vec3 c = 0.5 * (mn + mx);
            vec3 e = 0.5 * (mx - mn);
            vec3 q = abs(p - c) - e;
            float bd = length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
            if(op == 1 && bd >= d) continue;
            if(op == 2 && bd > 0.0 && d > -bd) continue;
        }
        if(kind == 16){
            if(have) d = d - max(0.05, a1.x);
            continue;
        }
        if(kind == 19){
            if(!have) continue;
            vec3 n = a1.xyz;
            if(dot(n,n) < 1e-8) n = vec3(0,0,1);
            n = normalize(n);
            d = max(d, dot(p, n) - a1.w);
            continue;
        }
        if(kind == 17) continue;
        if(kind == 23){
            if(!have) continue;
            vec3 A = a2.xyz;
            vec3 B = a1.xyz;
            float r = a2.w;
            vec3 n1 = texelP(b+3).xyz;
            vec3 n2 = texelP(b+13).xyz;
            float chamf = (cham > 0.5 || a4.x > 0.5) ? 1.0 : 0.0;
            vec4 a5e = texelP(b+5);
            int toff = int(a5e.z + 0.5), tn = int(a5e.w + 0.5);
            if(tn >= 3){
                for(int k=1; k<48; ++k){
                    if(k >= tn-1) break;
                    vec3 pa = curve(toff+k);
                    vec3 pb = curve(toff+k+1);
                    vec3 cr = cross(pb - pa, n1);
                    vec3 n2s = n2;
                    if(dot(cr, cr) > 1e-10){
                        n2s = normalize(cr);
                        if(dot(n2s, n2) < 0.0) n2s = -n2s;
                    }
                    d = edgeBlend(d, p, pa, pb, n1, n2s, r, chamf);
                }
            } else {
                d = edgeBlend(d, p, A, B, n1, n2, r, chamf);
            }
            continue;
        }
        if(kind == 10){
            if(!have) continue;
            float t = max(0.15, a1.x);
            d = max(d, -d - t);
            int face = int(a4.y + 0.5);
            vec3 c = 0.5 * (mn + mx);
            vec3 e = 0.5 * (mx - mn) + vec3(t);
            vec3 o = c, h = e;
            if(face==0){ o.z = mx.z; h.z = t*2.0; }
            else if(face==1){ o.z = mn.z; h.z = t*2.0; }
            else if(face==2){ o.x = mx.x; h.x = t*2.0; }
            else if(face==3){ o.x = mn.x; h.x = t*2.0; }
            else if(face==4){ o.y = mx.y; h.y = t*2.0; }
            else { o.y = mn.y; h.y = t*2.0; }
            d = max(d, -sdBox(p - o, h));
            continue;
        }
        vec4 r0 = texelP(b+9);
        vec4 r1 = texelP(b+10);
        vec4 r2 = texelP(b+11);
        vec4 r3 = texelP(b+12);
        vec4 lp4 = vec4(p, 1.0);
        vec3 lp = vec3(dot(r0, lp4), dot(r1, lp4), dot(r2, lp4));
        float pd = 1e9;
        vec3 sz = a1.xyz; float height = a1.w;
        float rnd = a2.w; float taper = texelP(b+3).w;
        int style = int(a4.x + 0.5);
        vec4 a5 = texelP(b+5);
        vec4 a6 = texelP(b+6);
        int poff = int(a5.x + 0.5), pn = int(a5.y + 0.5);
        int toff = int(a5.z + 0.5), tn = int(a5.w + 0.5);
        int boff = int(a6.x + 0.5), bn = int(a6.y + 0.5);
        if(kind==0){
            vec3 h = sz * 0.5;
            vec3 q = lp - h;
            pd = rnd > 0.02 ? sdRoundBox(q, h, rnd) : sdBox(q, h);
        } else if(kind==1) pd = sdSphere(lp, sz.x);
        else if(kind==2) pd = sdCyl(lp, sz.x, sz.y);
        else if(kind==3) pd = sdCone(lp, sz.x, sz.y);
        else if(kind==4) pd = sdExtrude(lp, poff, pn, height, taper, style==1);
        else if(kind==5) pd = sdPoly(vec2(length(lp.xy), lp.z), poff, pn);
        else if(kind==6) pd = sdSweep(lp, toff, tn, max(sz.x, rnd), poff, pn);
        else if(kind==7) pd = sdCoil(lp, sz.x, sz.y, sz.z, height > 0.1 ? height : 4.0);
        else if(kind==8) pd = sdSlot(lp, sz.x, sz.y, sz.z);
        else if(kind==9){
            float t = clamp(lp.z / max(height, 1e-3), 0.0, 1.0);
            float da = sdPoly(lp.xy, poff, pn);
            float db = sdPoly(lp.xy, boff, bn);
            pd = sdExtrude2(mix(da, db, t), lp.z, height);
        } else if(kind==11) pd = sdGear(lp, sz.x, sz.z, int(height + 0.5), sz.y > 0.2 ? sz.y : 8.0);
        else if(kind==12) pd = sdThread(lp, sz.x, sz.y, sz.z, rnd, style==1);
        else if(kind==13) pd = sdRib(lp, sz.x, sz.z, sz.y);
        else if(kind==14){
            float sw = height > 5.0 ? height : (style==1 ? 90.0 : style==2 ? 180.0 : 360.0);
            pd = sdTorus(lp, sz.x, sz.y, sw);
        } else if(kind==15){
            float ro = max(sz.x, rnd);
            float outer = sdSweep(lp, toff, tn, ro, poff, pn);
            if(sz.y > 0.08 && sz.y < ro - 0.08)
                pd = max(outer, -sdSweep(lp, toff, tn, sz.y, 0, 0));
            else pd = outer;
        } else if(kind==18) pd = sdHex(lp, sz.x, sz.y);
        else if(kind==21) pd = sdCapsule(lp, sz.x, sz.y);
        else if(kind==22) pd = sdWedge(lp, sz, style);
        else if(kind==20){
            float d2 = 1e9;
            for(int i=0;i<80;i+=2){
                if(i+1>=pn) break;
                vec2 a = curve(poff+i).xy;
                vec2 b = curve(poff+i+1).xy;
                vec2 e = b - a;
                float el = dot(e,e);
                float t = el > 1e-10 ? clamp(dot(lp.xy - a, e) / el, 0.0, 1.0) : 0.0;
                d2 = min(d2, length(a + e * t - lp.xy));
            }
            d2 = d2 - max(0.06, sz.y);
            pd = sdExtrude2(d2, lp.z, max(0.12, height));
        }
        if(!have || op==0){ d = pd; have = true; }
        else d = combine(op, d, pd, blend, cham);
        if(abs(pd) < abs(d) + 0.8) fid = a4.z;
        if(op==0) fid = a4.z;
    }
    HitInfo hi; hi.d = have ? d : 1e9; hi.fid = fid; return hi;
}

vec3 gradN(vec3 p, float t){
    float e = max(0.016, t * 0.0002);
    vec2 k = vec2(1.0, -1.0);
    return normalize(
        k.xyy * mapScene(p + k.xyy * e).d +
        k.yyx * mapScene(p + k.yyx * e).d +
        k.yxy * mapScene(p + k.yxy * e).d +
        k.xxx * mapScene(p + k.xxx * e).d);
}

bool rayBox(vec3 ro, vec3 rd, out float tN, out float tF){
    vec3 invrd = 1.0 / max(abs(rd), vec3(1e-6)) * sign(rd);
    vec3 ta = (uBMin - ro) * invrd;
    vec3 tb = (uBMax - ro) * invrd;
    vec3 tsm = min(ta, tb), tlg = max(ta, tb);
    tN = max(max(tsm.x, tsm.y), tsm.z);
    tF = min(min(tlg.x, tlg.y), tlg.z);
    return tF >= 0.0 && tN <= tF;
}

void makeRay(vec2 pix, out vec3 ro, out vec3 rd){
    vec2 ndc = (pix / uRes) * 2.0 - 1.0;
    vec4 a = uInvVP * vec4(ndc, -1.0, 1.0);
    vec4 b = uInvVP * vec4(ndc,  1.0, 1.0);
    ro = a.xyz / a.w;
    rd = normalize(b.xyz / b.w - ro);
}

struct March {
    bool hit;
    float t;
    float fid;
    float closest;
    float tclose;
};

March march(vec3 ro, vec3 rd){
    March r;
    r.hit = false; r.t = uFar; r.fid = 0.0; r.closest = 1e9; r.tclose = 0.0;
    float tN, tF;
    if(!rayBox(ro, rd, tN, tF)) return r;
    float t = max(tN, 0.0);
    float last = 0.0;
    float lim = min(uFar, tF + 2.0);
    for(int i = 0; i < 280; ++i){
        vec3 p = ro + rd * t;
        if(uSection == 1 && dot(p, uClip.xyz) > uClip.w){
            t += 0.22;
            if(t > lim) break;
            continue;
        }
        HitInfo h = mapScene(p);
        if(h.d < r.closest){ r.closest = h.d; r.tclose = t; }
        float th = max(0.00005 * t, 0.00045);
        if(h.d < th){
            float t0 = max(t - max(last, th * 4.0), tN);
            float t1 = t;
            for(int k = 0; k < 10; ++k){
                float tm = 0.5 * (t0 + t1);
                if(mapScene(ro + rd * tm).d < 0.0) t1 = tm; else t0 = tm;
            }
            r.hit = true;
            r.t = t1;
            r.fid = h.fid;
            return r;
        }
        last = clamp(h.d * 0.90, th * 0.35, 5.5);
        t += last;
        if(t > lim) break;
    }
    return r;
}

vec3 lightSolid(vec3 n, vec3 V, float fid){
    vec3 L1 = normalize(vec3(0.55, 0.20, 0.90));
    vec3 L2 = normalize(vec3(-0.65,-0.30, 0.35));
    vec3 L3 = normalize(vec3(0.05, 0.85, 0.25));
    float spec = pow(max(dot(n, normalize(L1 + V)), 0.0), 128.0) * 0.32;
    float ndl = 0.22 + 0.54*max(dot(n,L1),0.0) + 0.16*max(dot(n,L2),0.0) + 0.11*max(dot(n,L3),0.0);
    float ao = 0.86 + 0.14 * max(n.z, 0.0);
    float rim = pow(1.0 - max(dot(n, V), 0.0), 3.2) * 0.10;
    vec3 col = uColor * ndl * ao + vec3(0.94,0.96,0.99)*spec + vec3(0.78,0.84,0.94)*rim;
    uint ufid = uint(fid + 0.5);
    if(ufid == uSel && uSel != 0u) col = mix(col, vec3(1.0, 0.70, 0.18), 0.38);
    else if(ufid == uHover && uHover != 0u) col = mix(col, vec3(0.45, 0.78, 1.0), 0.24);
    return col;
}

vec3 inkContours(vec3 col, vec3 n, vec3 V, bool wire){
    float ndv = abs(dot(n, V));
    float fw = max(fwidth(ndv), 1e-5);
    // Thin Inventor rim — wide ndv bands fill holes with ink.
    float sil = 1.0 - smoothstep(0.0, max(0.022, fw * 2.0), ndv);
    float crease = smoothstep(0.045, 0.14, length(fwidth(n)));
    float edge = clamp(max(sil, crease * 0.85), 0.0, 1.0);
    vec3 ink = vec3(0.04, 0.045, 0.05);
    if(wire){
        float cover = smoothstep(0.12, 0.70, edge);
        return mix(vec3(0.0), ink, cover);
    }
    return mix(col, ink, edge);
}

struct Shade {
    vec4 rgba;
    float depth;
    float ndv;
};

Shade shadeRay(vec2 pix){
    Shade s;
    s.rgba = vec4(0.0);
    s.depth = 1.0;
    s.ndv = 1.0;
    vec3 ro, rd;
    makeRay(pix, ro, rd);
    March h = march(ro, rd);
    if(h.hit){
        vec3 pos = ro + rd * h.t;
        vec3 n = gradN(pos, h.t);
        vec3 V = normalize(uEye - pos);
        if(dot(n, V) < 0.0) n = -n;
        s.ndv = abs(dot(n, V));
        vec3 col = lightSolid(n, V, h.fid);
        if(uContours == 1 || uWire == 1) col = inkContours(col, n, V, uWire == 1);
        float alpha = 1.0;
        if(uWire == 1){
            float fw = max(fwidth(s.ndv), 1e-5);
            float sil = 1.0 - smoothstep(0.0, max(0.022, fw * 2.0), s.ndv);
            float crease = smoothstep(0.045, 0.14, length(fwidth(n)));
            alpha = smoothstep(0.10, 0.65, max(sil, crease));
        }
        vec4 clip = uVP * vec4(pos, 1.0);
        s.depth = clip.z / clip.w * 0.5 + 0.5;
        s.rgba = vec4(col, alpha);
        return s;
    }
    return s;
}

void main(){
    const vec2 tap[4] = vec2[](
        vec2( 0.125, -0.375),
        vec2(-0.375, -0.125),
        vec2( 0.375,  0.125),
        vec2(-0.125,  0.375));

    Shade a = shadeRay(gl_FragCoord.xy + tap[0]);
    bool edge = a.rgba.a < 0.97 || a.ndv < 0.28;
    vec3 pre = a.rgba.rgb * a.rgba.a;
    float cov = a.rgba.a;
    float dep = a.rgba.a > 0.5 ? a.depth : 1.0;
    int n = 1;
    if(edge){
        for(int i = 1; i < 4; ++i){
            Shade b = shadeRay(gl_FragCoord.xy + tap[i]);
            pre += b.rgba.rgb * b.rgba.a;
            cov += b.rgba.a;
            if(b.rgba.a > 0.5) dep = min(dep, b.depth);
            n += 1;
        }
    }
    float alpha = cov / float(n);
    if(alpha < 0.004){
        gl_FragDepth = 1.0;
        frag = vec4(0.0);
        return;
    }
    vec3 rgb = pre / max(cov, 1e-4);
    gl_FragDepth = dep < 1.0 ? dep : a.depth;
    frag = vec4(rgb, clamp(alpha, 0.0, 1.0));
}
)";
