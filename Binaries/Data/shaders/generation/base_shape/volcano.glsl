{
	"Name": "Volcano",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Height",
			"Type": "Float",
			"Default": 1.5,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},    
		{
			"Name": "Radius",
			"Type": "Float",
			"Default": 0.7,
			"Widget": "Slider",
			"Constraints": [0.2, 1.0, 0.0, 0.0]
		},
		{
			"Name": "MountainFalloff",
			"Type": "Float",
			"Default": 0.2,
			"Label": "Mountain Falloff",
			"Widget": "Slider",
			"Constraints": [0.01, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Distortion",
			"Type": "Float",
			"Default": 0.1,
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "DistortionScale",
			"Type": "Float",
			"Default": 1.0,
			"Label": "Distortion Scale",
			"Widget": "Slider",
			"Constraints": [0.0, 8.0, 0.0, 0.0]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 12,
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
			"Name": "OutsideNoise",
			"Type": "Float",
			"Default": 0.2,
			"Label": "Outside Noise",
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "OutsideNoiseScale",
			"Type": "Float",
			"Default": 5.0,
			"Label": "Outside Noise Scale",
			"Widget": "Slider",
			"Constraints": [0.0, 16.0, 0.0, 0.0]
		},
		{
			"Name": "CraterRadius",
			"Type": "Float",
			"Default": 1.0,
			"Label": "Crater Radius",
			"Widget": "Slider",
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "CraterDepth",
			"Type": "Float",
			"Default": 2.0,
			"Label": "Crater Depth",
			"Widget": "Slider",
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		}
	]
}
// CODE


vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*(43758.5453123 + u_Seed));
}

float snoise( in vec2 p )
{
		const float K1 = 0.366025404; // (sqrt(3)-1)/2;
		const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
		vec2  a = p - i + (i.x+i.y)*K2;
		float m = step(a.y,a.x); 
		vec2  o = vec2(m,1.0-m);
		vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
		vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
		return dot( n, vec3(70.0) );
}

vec2 random2(vec2 p) 
{
    vec3 p3 = fract(p.xyx * vec3(.1031, .1030, .0973) * (u_Seed + 10));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}

float smin( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*k*(1.0/4.0);
}

float noise(vec2 uv)
{
	const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float fac = 1.0f;
	float f  = fac * 0.5000f * snoise( uv ); uv = m*uv;
	f += fac * 0.2500f * snoise( uv ); uv = m*uv;
	f += fac * 0.1250f * snoise( uv ); uv = m*uv;
	f += fac * 0.0625f * snoise( uv ); uv = m*uv;
	return f;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	uv = uv * 2.0f - vec2(1.0) + u_Offset;
	float distortion = noise(uv * u_DistortionScale) * u_Distortion;
	float r = length(uv) + distortion;
	float craterRadius = u_Radius * u_CraterRadius * 0.3f;
	float ns = u_Height * smoothstep(u_Radius + u_MountainFalloff, craterRadius * 0.5f, r) - u_Height * u_CraterDepth * 0.3f * smoothstep(craterRadius, 0.0f, r);
	ns += snoise(uv * u_OutsideNoiseScale) * 0.2f * u_OutsideNoise * smoothstep(0.0, craterRadius, r);
	return ns * u_Strength;
}