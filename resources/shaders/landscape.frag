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

float is_lit2() {
    vec3 current = vec3(surf.texCoord.x, textureLod(heightmap, surf.texCoord, 0.0f).x, surf.texCoord.y);
    const float h = Params.initial_trace_step;
    const vec3 dir = normalize(Params.lightPos - current);

    float result = 0;
    int iterations = 1000;

    while (current.x >=  0 && current.x <= 1
        && current.y >= -1 && current.y <= 1
        && current.z >=  0 && current.z <= 1 && iterations > 0)
    {
        iterations--;
        current += h*dir;

        if (textureLod(heightmap, current.xz, 0).r > current.y)
            result += 1.f;
    }

    const float minHits = 10;
    return (minHits - min(result, minHits))/minHits;
}
float is_lit() {
    float step = Params.initial_trace_step;
    //vec3 current_pos = vec3(surf.texCoord.x, textureLod(heightmap, surf.texCoord, 0.0f).x, surf.texCoord.y);
    vec3 current_pos = surf.wPos + vec3(0, 0.01, 0);
    vec3 light_dir = normalize(Params.lightPos - current_pos);
    int i = 1000;
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
        //float cur_height = calc_height(tex_pos);
        float cur_height = textureLod(heightmap, current_pos.xz, 0).r;
        if (cur_height > current_pos.y) {
            hits += 1.0f;
        }
        //step *= 1.01f;
    }
    const float minHits = 10.0f;
    return max((minHits - min(hits, minHits))/minHits, 0.2);
    //return dot(light_dir, surf.sNorm);
}

void main()
{
    int color_type = 0;
    if (color_type == 0) {
        //vec4 light_color = vec4(Params.lightColor, 1.0f);
        vec4 light_color = vec4(1.0f);
        fragColor = is_lit2() * light_color;
    } else if (color_type == 1) {
        fragColor = vec4(surf.sNorm, 1.0);
    } else if (color_type == 2) {
        fragColor = vec4(texture(heightmap, surf.texCoord).r);
    } else {
        fragColor = vec4(1.0f, 0, 0, 0.0f);
    }
}


