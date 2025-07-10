#pragma once

//Allows conversion between stuff like ImVec2 and Vector2i
// define glm::vecN or include it from another file
namespace glm
{
    struct vec2
    {
        float x, y;
        vec2(float x, float y) : x(x), y(y) {};
    };
    struct vec4
    {
        float x, y, z, w;
        vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {};
    };
}
#define _USE_MATH_DEFINES
// define extra conversion here before including imgui, don't do it in the imconfig.h
#define IM_VEC2_CLASS_EXTRA \
    constexpr ImVec2(glm::vec2& f) : x(f.x), y(f.y) {} \
    operator glm::vec2() const { return glm::vec2(x, y); }

#define IM_VEC4_CLASS_EXTRA \
        constexpr ImVec4(const glm::vec4& f) : x(f.x), y(f.y), z(f.z), w(f.w) {} \
        operator glm::vec4() const { return glm::vec4(x,y,z,w); }