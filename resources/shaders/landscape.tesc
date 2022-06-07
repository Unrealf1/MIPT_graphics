#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#include "common.h"


layout(push_constant) uniform params_t
{
    mat4 mProj;
    mat4 mView;
} params;

layout(binding = 0, set = 0) uniform AppData
{
    UniformParams Params;
};

layout(binding = 1, set = 0) uniform sampler2D heightmap;

layout(vertices = 4) out;

void main()
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = Params.grid_size;
        gl_TessLevelInner[1] = Params.grid_size;

        gl_TessLevelOuter[0] = Params.grid_size;
        gl_TessLevelOuter[1] = Params.grid_size;
        gl_TessLevelOuter[2] = Params.grid_size;
        gl_TessLevelOuter[3] = Params.grid_size;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
