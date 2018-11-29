R"(
#version 330 core
#extension GL_ARB_gpu_shader_fp64 : enable

in vec2 Position;

out vec4 color;

uniform double zoomLevel;
uniform int MaxIterations;
uniform double camPointX;
uniform double camPointY;

void main()
{
    double real  = (double(Position.x) / double(pow(10, float(zoomLevel))) + double(camPointX)) * 1.8;
    double imag  = double(Position.y) / double(pow(10, float(zoomLevel))) + double(camPointY);
    double Creal = real;
    double Cimag = imag;

    double r2 = 0.0;
	float quotient;

	float R, G, B;

	color = vec4(0.0, 0.0, 0.0, 1.0);

    for (float iter = 0.0; iter < MaxIterations; ++iter)
    {
        double tempreal = real;

        real = (tempreal * tempreal) - (imag * imag) + Creal;
        imag = 2.0 * tempreal * imag + Cimag;
        r2 = (real * real) + (imag * imag);

		if (r2 >= 2.0) {
			quotient = iter / MaxIterations;
			if (quotient < 0) quotient = 0;
			if (quotient < 0.16) {
				R = 32 * (quotient / 0.16);
				G = 100 * (quotient / 0.16) + 7;
				B = 103 * (quotient / 0.16) + 100;
			}
			else if (quotient < 0.42) {
				R = 205 * ((quotient - 0.16) / 0.26) + 32;
				G = 148 * ((quotient - 0.16) / 0.26) + 107;
				B = 52 * ((quotient - 0.16) / 0.26) + 203;
			}
			else if (quotient < 0.6425) {
				R = 18 * ((quotient - 0.42) / 0.2225) + 237;
				G = 255 - 85 * ((quotient - 0.42) / 0.2225);
				B = 255 - 255 * ((quotient - 0.16) / 0.2225);
			}
			else if (quotient < 0.8575) {
				R = 255 - 255 * ((quotient - 0.6425) / 0.2150);
				G = 170 - 168 * ((quotient - 0.6425) / 0.2150);
				B = 0;
			}
			else {
				R = 0;
				G = 5 * ((quotient - 0.8575) / 0.1425) + 2;
				B = 100 * ((quotient - 0.8575) / 0.1425);
			}
			color = vec4(R / 255, G / 255, B / 255, 1.0);
			break;
		}
    }
}
)"