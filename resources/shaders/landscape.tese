#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout(push_constant) uniform params_t
{
    mat4 mProj;
    mat4 mView;
} params;

layout(binding = 1, set = 0) uniform sampler2D heightmap;

layout(binding = 0, set = 0) uniform AppData
{
    UniformParams Params;
};

layout(quads, equal_spacing, cw) in;

layout (location = 0) out VS_OUT
{
    vec3 cNorm;
    vec2 texCoord;
    vec3 wPos;
} vOut;

float calcHeight(vec2 pos)
{
    float eps = 1.0 / Params.landscape_length;
    return textureLod(heightmap, eps + pos / (eps + Params.landscape_length), 0).r * 15.0 - 20.0;
}

vec3 calcNormal(vec2 pos)
{
    const float eps = Params.normal_calc_eps;
    const vec2 dx = vec2(eps, 0);
    const vec2 dy = vec2(0, eps);
    const float r = calcHeight(pos + dx);
    const float l = calcHeight(pos - dx);
    const float u = calcHeight(pos + dy);
    const float d = calcHeight(pos - dy);

    return normalize(vec3(r - l, 0.01f, d - u));
}

vec3 calcPos(vec2 pos)
{
    return vec3(pos.x - Params.landscape_length / 2, calcHeight(pos), pos.y - Params.landscape_length / 2);
}

void main()
{
    const vec2 mPos2 = gl_TessCoord.xy * Params.landscape_length;

    const vec3 mPos = calcPos(mPos2);
    const vec3 mNorm = calcNormal(mPos2);

    mat4 modelView = params.mView;

    mat4 normalModelView = transpose(inverse(modelView));

    vOut.cNorm = normalize(mat3(normalModelView) * mNorm);
    vOut.texCoord = mPos2 / Params.landscape_length;
    vOut.wPos = mPos;
    
    gl_Position = params.mProj * modelView * vec4(mPos, 1);
}
