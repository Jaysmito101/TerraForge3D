{
	"Name": "Dunes",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},    
		{
			"Name": "Smoothness",
			"Type": "Float",
			"Default": 0.14,
			"Widget": "Slider",
			"Label": "Edge Smoothness",
			"Constraints": [0.001, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 42,
			"Widget": "Seed"
		},
		{
			"Name": "Offset",
			"Type": "Vector2",
			"Default": [0.0, 0.0],
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Distortion",
			"Type": "Float",
			"Default": 0.2,
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "DistortionScale",
			"Type": "Float",
			"Default": 0.4,
			"Widget": "Slider",
			"Label": "Distortion Scale",
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		}
	]
}
// CODE
#include "common/noise_2d.glsl"


float noise(vec2 uv)
{
	const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float fac = 1.0f;
	float f  = fac * 0.5000f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.2500f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.1250f * tf3d_snoise2( uv ); uv = m*uv;
	f += fac * 0.0625f * tf3d_snoise2( uv ); uv = m*uv;
	return f;
}


float evaluateBaseShape(vec2 uv, vec3 seed)
{
	vec2 p = uv * u_Scale * 4.0f + u_Offset;
	vec2 n = floor( p );
    vec2 f = fract( p );

    float ns = 1.0;
    for( int j= -1; j <= 1; j++ ) for( int i=-1; i <= 1; i++ )
	{	
        vec2  g = vec2(i,j);
        vec2  o = random2( n + g );
        vec2  delta = g + o - f + vec2(tf3d_snoise2(uv * u_DistortionScale) * u_Distortion);
        float d = smoothstep(0.01f, 1.0f, length(delta));
        ns = smin(ns, d, u_Smoothness);
    }

	//float n = fabs(noise(val, nl.depth));
	return ns * u_Strength;
}