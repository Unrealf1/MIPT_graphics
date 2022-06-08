#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout (location = 0) in VS_OUT
{
    vec3 sNorm;
    vec2 texCoord;
    vec3 wPos;
} surf;

layout(binding = 0, set = 0) uniform AppData
{
    UniformParams Params;
};

layout (location = 0) out vec4 fragColor; 
layout(binding = 1, set = 0) uniform sampler2D heightmap;

float calc_height(vec2 pos)
{
    float eps = 1.0 / Params.landscape_length;
    return textureLod(heightmap, eps + pos / (Params.landscape_length + eps), 0).r * 15.0 - 20.0;
}

float is_lit() {
    float step = Params.initial_trace_step;
    vec3 current_pos = surf.wPos + vec3(0, 0.01, 0);
    vec3 light_dir = normalize(Params.lightPos - current_pos);
    int i = 500;
    float hits = 0.0f;
    while (i > 0) {
        current_pos += light_dir * step;
        i--;
        float max_coord = max(current_pos.x, current_pos.z);
        float min_coord = min(current_pos.x, current_pos.z);
        if (max_coord > Params.landscape_length / 2) {
            break;
        }
        if (min_coord < -Params.landscape_length / 2) {
            break;
        }
        if (abs(current_pos.y) > 30) {
            break;
        }

        vec2 tex_pos = vec2(current_pos.x, current_pos.z) + vec2(Params.landscape_length / 2);
        float cur_height = calc_height(tex_pos);
        //float cur_height = textureLod(heightmap, current_pos.xz, 0).r;
        if (cur_height > current_pos.y) {
            hits += 1.0f;
        }
        step *= 1.01f;
    }
    const float minHits = 10.0f;
    float shade_power = max((minHits - min(hits, minHits))/minHits, 0.2);
    return max(dot(light_dir, surf.sNorm), 0) * shade_power;
}

void main()
{
    int color_type = 0;
    if (color_type == 0) {
        //vec4 light_color = vec4(Params.lightColor, 1.0f);
        vec4 light_color = vec4(1.0f);
        fragColor = is_lit() * light_color;
    } else if (color_type == 1) {
        fragColor = vec4(surf.sNorm, 1.0);
    } else if (color_type == 2) {
        fragColor = vec4(texture(heightmap, surf.texCoord).r);
    } else {
        fragColor = vec4(1.0f, 0, 0, 0.0f);
    }
}


