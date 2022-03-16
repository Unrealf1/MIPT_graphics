#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
} surf;

layout(binding = 0, set = 0) uniform AppData
{
  CustomUniformParams Params;
};

layout (binding = 1) uniform sampler2D shadowMap;
layout (binding = 2) uniform sampler2D shadowMapAdditional;

void main()
{
  const vec4 posLightClipSpace = Params.lightMatrix*vec4(surf.wPos, 1.0f); // 
  const vec3 posLightSpaceNDC  = posLightClipSpace.xyz/posLightClipSpace.w;    // for orto matrix, we don't need perspective division, you can remove it if you want; this is general case;
  const vec2 shadowTexCoord    = posLightSpaceNDC.xy*0.5f + vec2(0.5f, 0.5f);  // just shift coords from [-1,1] to [0,1]               
    
  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0091f || shadowTexCoord.y > 0.9999f);
  float shadow = 0.0;
  if (Params.use_variance == 1) {
    const vec2 pair = textureLod(shadowMapAdditional, shadowTexCoord, 0).xy; 
    const float M1 = pair.x;
    const float M2 = pair.y;
    const float M1_2 = M1 * M1;
    const float sig = max(M2 - M1_2, 0.0001);

    const float t = posLightSpaceNDC.z;
    const float p = t < M1 ? 1.0 : 0.0;
    const float diff = t - M1;
    const float diff_2 = diff * diff;
    const float pmax = sig / (sig + diff_2);

    shadow = max(p, pmax);
  } else {
    shadow = ((posLightSpaceNDC.z < textureLod(shadowMap, shadowTexCoord, 0).x + 0.001f) || outOfView) ? 1.0f : 0.0f;
  }

  const vec4 dark_violet = vec4(0.59f, 0.0f, 0.82f, 1.0f);
  const vec4 chartreuse  = vec4(0.5f, 1.0f, 0.0f, 1.0f);

  vec4 lightColor1 = mix(dark_violet, chartreuse, abs(sin(Params.time)));
  vec4 lightColor2 = vec4(1.0f, 1.0f, 1.0f, 1.0f);
   
  vec3 dir_to_light   = normalize(Params.lightPos - surf.wPos);
  float light_cos = dot(-dir_to_light, normalize(Params.lightDir));
  float angle = acos(light_cos);
  if (angle > Params.spread) {
    out_fragColor = vec4(0.0);
    return;
  }
  float dimming = 1.0 - angle/Params.spread;
  vec4 lightColor = max(dot(surf.wNorm, dir_to_light), 0.0f) * lightColor1;
  out_fragColor   = dimming * (lightColor*shadow + vec4(0.1f)) * vec4(Params.baseColor, 1.0f);
}
