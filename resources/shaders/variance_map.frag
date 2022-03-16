#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout (location = 0) out vec2 out_fragColor;
layout (binding = 0) uniform sampler2D shadowMap;

layout(binding = 3) uniform AppData
{
  ShadowmapAdditionalParams Params;
};

void filter_mean(vec2 point, vec2 win_dims, float step, out float x, out float x_2) {
    float result = 0;
    float square = 0;

    vec2 halfdims = win_dims / 2.0;
    vec2 tex_dims = vec2(textureSize(shadowMap, 0));

    for (float i = point.x - halfdims.x; i <= point.x + halfdims.x; i += step) {
        for (float j = point.y - halfdims.y; j <= point.y + halfdims.y; j += step) {
            float current = textureLod(shadowMap, vec2(i, j) / tex_dims, 0).x;
            result += current;
            square += current * current;
        }
    }
    float n = (win_dims.x + 1) * (win_dims.x + 1);
    x = result / n;
    x_2 = square / n;       
}

void main()
{
    float depth;
    float square;

    filter_mean(gl_FragCoord.xy, Params.kernel_dims, 1.0,  depth, square);
    out_fragColor = vec2(depth, square); 
} 

