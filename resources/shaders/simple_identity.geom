#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

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
    // emit original triangle
    for (int i = 0; i < 3; i++)
    {
        gl_Position = gl_in[i].gl_Position;

        gsOut.wNorm = gsIn[i].wNorm;
        gsOut.wTangent = gsIn[i].wTangent;
        gsOut.texCoord = gsIn[i].texCoord;
        EmitVertex();
    }

    EndPrimitive();
}