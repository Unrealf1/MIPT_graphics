#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout (location = 0) in VS_OUT
{
    vec3 sNorm;
    vec2 texCoord;
} surf;


layout(binding = 0, set = 0) uniform AppData
{
    UniformParams Params;
};

layout (location = 0) out vec4 fragColor; 
layout(binding = 1, set = 0) uniform sampler2D heightmap;

void main()
{
    fragColor = vec4(1.0, 0, 0, 1.0);
    fragColor = vec4(surf.sNorm, 1.0);
    fragColor = vec4(texture(heightmap, surf.texCoord).r);
}


