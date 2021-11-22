#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;
layout (line_strip, max_vertices = 2) out;

in gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
} gl_in[];


out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

layout (location = 0 ) in GS_IN
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
    vec3 trueNorm;
} gsIn[];

layout (location = 0 ) out GS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} gsOut;

void main()
{
    vec3 normal = vec3(0);
    normal /= 3;
    vec4 base = gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position;
    base /= 3;

    gl_Position = base;
    gsOut.wNorm = gsIn[0].wNorm;
    gsOut.wTangent = gsIn[0].wTangent;
    gsOut.texCoord = gsIn[0].texCoord;
    EmitVertex();

    vec3 d21 = vec3(gl_in[1].gl_Position - gl_in[0].gl_Position);
    vec3 d31 = vec3(gl_in[2].gl_Position - gl_in[0].gl_Position);
    vec3 new_norm = vec3((d21.y * d31.z) - (d21.z * d31.y), (d21.z * d31.x) - (d21.x * d31.z), (d21.x * d31.y) - (d21.y * d31.x));
    new_norm /= length(new_norm);
    new_norm *= 0.15;

    gl_Position = base + vec4(new_norm, 0);
    gsOut.wNorm = gsIn[0].wNorm;
    gsOut.wTangent = gsIn[0].wTangent;
    gsOut.texCoord = gsIn[0].texCoord;
    EmitVertex();
    EndPrimitive();
    return;
}