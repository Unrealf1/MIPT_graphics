#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D colorTex;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

float calc_brightness(vec3 pixel) {
	return dot(pixel, vec3(0.299, 0.587, 0.144));
}

void main()
{
  // comment this two ifs to remove the red line dividing filtered and original image
  // and apply filter to the whole image
  const float divider = 0.5;
  if (abs(surf.texCoord.x - divider) <= 0.001) {
	color = vec4(1.0, 0.0, 0.0, 0.0);
	return;
  }

  if (surf.texCoord.x < divider) {
	color = vec4(vec3(textureLod(colorTex, surf.texCoord, 0)), 0);
	return;
  }

  ivec2 tex_size = textureSize(colorTex, 0);
  vec2 step = vec2(1) / vec2(tex_size);

  const int window_size = 4;
  const int halfsize = window_size / 2;
  const int buffer_size = window_size * window_size;
  // 4rth value is  brightness
  vec4 pixels[buffer_size];
  // read every element in thhe window and put them into a sorted array
  for (int i = 0; i < window_size; ++i) {
	for (int j = 0; j < window_size; ++j) {
		int element_number = i * window_size + j;
		vec3 current_color = vec3(textureLod(colorTex, surf.texCoord + step * vec2(i, j) - halfsize, 0));
		float current_brightness = calc_brightness(current_color);
		
		for (int ii = 0; ii <= element_number; ++ii) {
			// if there weren't elements greater
			if (ii == element_number) {
				pixels[element_number] = vec4(current_color, current_brightness);
				break;
			}

			if (current_brightness < pixels[ii].a) {
				// insert element here
				vec4 to_insert = vec4(current_color, current_brightness);
				for (int jj = ii; jj < element_number - 1; ++jj) {
					vec4 tmp = to_insert;
					to_insert = pixels[jj];
					pixels[jj] = tmp;
				}
				pixels[element_number] = to_insert;
				break;
			}
		}
	}
  }

  color = vec4(pixels[buffer_size / 2].rgb, 0);
  //vec4 original_color = vec4(vec3(textureLod(colorTex, surf.texCoord, 0)), 0);
  //color = abs(color - original_color);
}
