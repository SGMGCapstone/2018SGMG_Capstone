/*
#version 430
#extension GL_EXT_gpu_shader5 : enable

uniform sampler2D DSC_input;

in vec2 v_texCoord;
out vec4 outColor;

void main()
{
	float Color = 0.0;
	float WeightTemp;
	vec2 new_vec;

	new_vec = vec2(1.0 - v_texCoord.y, 1.0 - v_texCoord.x);

	if(new_vec.y < 0.0 || new_vec.y > 1.0 || new_vec.x < 0.0 || new_vec.x > 1.0)
	{
		Color = 0.0;
	}
	else
	{
		Color = texture(DSC_input, new_vec).x;
	}

	outColor = vec4(Color, Color, Color, 1.0);
}
*/

__kernel
void digital_scanconversion(
	__global float *bep_result,
	__global uchar *dsc_result,
	float dsc_factor,
	int max_width
)
{
	// scanline : 0
	if (get_global_id(0) >= max_width) return;

	int output_idx = get_global_id(1) * max_width + (max_width - get_global_id(0));
	float temp = convert_float(get_global_id(0) + 1);
	int _sample = get_global_id(1);

	//_scanline / dsc_scanline_size * 128.0
	float scanline_target = temp * dsc_factor;
	float temp_prev = floor(scanline_target);
	float temp_next = temp_prev + 1.0f;
	float weight_prev = (temp_next - scanline_target);
	float weight_next = 1.0f - weight_prev;
	int scanline_prev = convert_int(temp_prev) - 1; // -1 + 1한것
	int scanline_next = convert_int(temp_next) - 1; // -1 + 1한것

	if (scanline_next > 128)
	{
		dsc_result[output_idx] = convert_uchar(
			clamp(
				bep_result[scanline_prev * 1024 + _sample],
				0.0f,
				255.0f
			)
		);
	}
	else
	{
		dsc_result[output_idx] = convert_uchar(
			clamp(
				weight_prev * bep_result[scanline_prev * 1024 + _sample]
				+ weight_next * bep_result[scanline_next * 1024 + _sample],
				0.0f,
				255.0f
			)
		);
	}

	//printf("%d : %u\n", output_idx, dsc_result[output_idx]);
}