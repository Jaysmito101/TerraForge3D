{
  "Name": "Cliff",
  "Params": [
    {
      "Name": "Strength",
      "Type": "Float",
      "Default": 0.5,
      "Widget": "Drag",
      "Sensitivity": 0.01
    },
    {
      "Name": "Rotation",
      "Type": "Float",
      "Default": 0.0,
      "Widget": "Slider",
      "Constraints": [-180.00, 180.00, 0.0, 0.0]
    },
    {
      "Name": "Thickness",
      "Type": "Float",
      "Default": 0.550,
      "Widget": "Slider",
      "Constraints": [0.00, 1.0, 0.0, 0.0]
    },
    {
      "Name": "Position",
      "Type": "Float",
      "Default": 0.65,
      "Widget": "Drag",
      "Sensitivity": 0.01
    },
    {
      "Name": "Distortion",
      "Type": "Float",
      "Default": 0.230,
      "Widget": "Slider",
      "Constraints": [0.0, 1.0, 0.0, 0.0]
    },
    {
      "Name": "DistortionScale",
      "Type": "Float",
      "Default": 1.16,
      "Label": "Distortion Scale",
      "Widget": "Drag",
      "Sensitivity": 0.01      
    },
    {
      "Name": "Noise",
      "Type": "Float",
      "Default": 0.130,
      "Widget": "Slider",
      "Constraints": [0.0, 1.0, 0.0, 0.0]
    },
    {
      "Name": "NoiseScale",
      "Type": "Float",
      "Default": 0.7,
      "Label": "Noise Scale",
      "Widget": "Drag",
      "Sensitivity": 0.001,
      "Constraints": [-2.0, 2.0, 0.0, 0.0]
    }
  ]
}
// CODE
#include "common/noise_2d.glsl"

vec2 rotate(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float noise(vec2 uv)
{
	mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float f  = 0.5000f * tf3d_snoise2( uv ); uv = m*uv;
	f += 0.2500f * tf3d_snoise2( uv ); uv = m*uv;
	f += 0.1250f * tf3d_snoise2( uv ); uv = m*uv;
	f += 0.0625f * tf3d_snoise2( uv ); uv = m*uv;
	return f;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float rotation = 3.141f * u_Rotation / 180.0f;
	uv = 2.0f * uv - vec2(1.0f);
	uv = rotate(uv, rotation);
	float x = (1.0f - uv.x - u_Position - u_Distortion * noise(uv * u_DistortionScale)) / u_Thickness;
	float ns = pow(0.5f * x * x * x - 1.5f * x, 2.0f);
	if(x < 0.0f) ns = 0.0f;
	else if(x > 1.0f) ns = 1.0f;
	ns += noise(uv * u_NoiseScale + vec2(1.0f, 2.0f)) * u_Noise;
	return ns * u_Strength;
}