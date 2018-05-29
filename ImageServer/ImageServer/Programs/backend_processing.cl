float get_beam_count(float scanline_pos)
{
   if(scanline_pos < 16.0)
   {
       return 32.0 / (scanline_pos + 16.0);
   }
   else if(scanline_pos > (128 - 16.0))
   {
       return 32.0 / (16.0 + (128 - scanline_pos));
   }
   else
   {
       return 1.0;
   }
}

__kernel
void backend_processing(
	__global short2 *in_iq_data,
	__global float *out_bep_result,
	__global float *in_tgc_gain,
	float gain,
	float DR,
	float min,
	float logFactor)
{
	// x => _sample
	// y => _scanline
	int idx = get_global_id(1) * get_global_size(0) + get_global_id(0);
	int _sample = get_global_id(0);
	int _scanline = get_global_id(1);
	float tgc_gain = in_tgc_gain[_sample];

	//printf("%d : (%d, %d)\n", idx, in_iq_data[idx].x, in_iq_data[idx].y);

	float2 iq_float = (float2)(
		convert_float(in_iq_data[idx].x),
		convert_float(in_iq_data[idx].y)
	);
	iq_float *= get_beam_count(convert_float(_scanline));

	float envelope = clamp(tgc_gain * gain * length(iq_float), 1.0f, 65536.0f);
	float logCompression = logFactor * log(envelope / min);

	//out_bep_result[idx] = clamp(logCompression, 0.0f, 255.0f);

	float x = clamp(logCompression, 0.0f, 255.0f);
	x = (x * x) / 255.0f;
	out_bep_result[idx] = x;

	//printf("%d : %f / %f / %f / %f\n", idx, length(iq_float), envelope, logCompression, out_bep_result[idx]);
}

/*
float xmin = xmax / (pow(10.0, DR / 20.0));
float logCompression = ymax * 20.0 / DR * (log(envlope / xmin) / log(10.0));
*/