#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "unpack_attributes.h"


layout(location = 0) in vec4 vPosNorm;
layout(location = 1) in vec4 vTexCoordAndTang;

layout(push_constant) uniform params_t
{
    mat4 mProjView;
    mat4 mModel;
} params;


layout (location = 0 ) out VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
    vec4 flexColor; // because we are flexing with math!
} vOut;

vec4 get_flex_color(vec3 position) {
    return vec4(vec3(1.0, 1.0, 1.0), sin(abs(position.y - position.x)));
    float F = (sqrt(4) - 1) / 3;
    vec3 addition = vec3(position.x + position.y + position.z);
    float transparency = position.z - position.x * position.y;
    float transparency_mod = exp(1.0 + (1.0 + cos(position.x)) / 2.0);
    if (position.z > 6.0) {
        return vec4(sin(abs(position.y - position.x)));
    }
    return vec4(vec3(position + F * addition), (1.0 + sin(transparency)) / (transparency_mod));
}

out gl_PerVertex { vec4 gl_Position; };
void main(void)
{
    const vec4 wNorm = vec4(DecodeNormal(floatBitsToInt(vPosNorm.w)),         0.0f);
    const vec4 wTang = vec4(DecodeNormal(floatBitsToInt(vTexCoordAndTang.z)), 0.0f);

    vOut.wPos     = (params.mModel * vec4(vPosNorm.xyz, 1.0f)).xyz;
    vOut.wNorm    = mat3(transpose(inverse(params.mModel))) * wNorm.xyz;
    vOut.wTangent = mat3(transpose(inverse(params.mModel))) * wTang.xyz;
    vOut.texCoord = vTexCoordAndTang.xy;

    gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);

    vOut.flexColor = get_flex_color(vec3(gl_Position));
}
