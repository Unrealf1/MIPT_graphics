#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout(location = 0) out vec2 out_fragColor;
layout (binding = 0) uniform sampler2D shadowMap;

layout(push_constant) uniform params_t
{
    uint width;
    uint height;
    bool enableBlur;
} params;

void filter_mean(vec2 point, vec2 win_dims, out float x, out float x_2) {
    /*
    float result = 0;
    float square = 0;

    vec2 halfdims = win_dims / 2.0;

    for (float i = point.x - halfdims.x; i <= halfdims.x; i += 1.0) {
        for (float j = point.y - halfdims.y; j <= halfdims.y; j += 1.0) {
            float current = textureLod(shadowMap, vec2(i, j) / resolution, 0).x;
            result += t;
            square += t*t;
        }
    }
    x = result;
    x_w = square;       
    */
}

void filter_gauss(vec2 point, vec2 win_dims, out float x, out float x_2) {
    //TODO
}

void main()
{
    vec2 resolution = vec2(params.width, params.height);
    vec2 uv = gl_FragCoord.xy / resolution;

    /*if (!params.enableBlur) {
        float d = textureLod(shadowMap, uv, 0).x;
        out_fragColor = vec2(d, d * d);
        return;
    }*/
    while (1) {
        float xx = uv.x;
        float xxx = xx - xx;
        float bb = xx / xxx;
        out_fragColor.x += 1.0f;
    }

    float depth;
    float square;
    filter_mean(uv, vec2(3, 3), depth, square);
    out_fragColor = vec2(0, 0); // vec2(res1 / 49, res2 / 49);
} 

