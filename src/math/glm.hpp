#pragma once
// Simple GLM-compatible math library
#include <cmath>

namespace glm {
    struct vec2 { float x, y; vec2(float x=0, float y=0) : x(x), y(y) {} };
    struct vec3 { float x, y, z; vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {} };
    struct vec4 { float x, y, z, w; vec4(float x=0, float y=0, float z=0, float w=0) : x(x), y(y), z(z), w(w) {} };
    struct mat4 { float m[16]; mat4() { for(int i=0; i<16; i++) m[i] = (i%5==0) ? 1.0f : 0.0f; } };
    
    // Basic functions
    inline float length(const vec2& v) { return std::sqrt(v.x*v.x + v.y*v.y); }
    inline vec2 normalize(const vec2& v) { float l = length(v); return l > 0 ? vec2(v.x/l, v.y/l) : vec2(0,0); }
    inline vec2 mix(const vec2& a, const vec2& b, float t) { return vec2(a.x + t*(b.x-a.x), a.y + t*(b.y-a.y)); }
    inline float clamp(float x, float min, float max) { return x < min ? min : (x > max ? max : x); }
}
