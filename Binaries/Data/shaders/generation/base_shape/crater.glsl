{
	"Name": "Crater",
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
			"Name": "Position",
			"Type": "Vector2",
			"Default": [0.0, 0.0],
			"Widget": "Slider",
			"Constraints": [-1.00, 1.00, 0.0, 0.0]
		},
		{
			"Name": "Radius",
			"Type": "Float",
			"Default": 0.4,
			"Widget": "Slider",
			"Constraints": [0.03, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Depth",
			"Type": "Float",
			"Default": 0.634,
			"Widget": "Slider",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "InnerFalloff",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Label": "Inner Falloff",
			"Constraints": [0.0, 0.999, 0.0, 0.0]
		},
		{
			"Name": "Height",
			"Type": "Float",
			"Default": 0.179,
			"Widget": "Slider",
			"Constraints": [0.00, 1.0, 0.0, 0.0]
		},
		{
			"Name": "OuterFalloff",
			"Type": "Float",
			"Default": 0.8,
			"Widget": "Slider",
			"Label": "Outer Falloff",
			"Constraints": [0.01, 1.0, 0.0, 0.0]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 0,
			"Widget": "Seed"
		},
		{
			"Name": "LargeDistortion",
			"Type": "Float",
			"Default": 0.2,
			"Widget": "Slider",
			"Label": "Large Distortion",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "SmallDistortion",
			"Type": "Float",
			"Default": 0.7,
			"Widget": "Slider",
			"Label": "Small Distortion",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "LargeNoise",
			"Type": "Float",
			"Default": 0.2,
			"Widget": "Slider",
			"Label": "Large Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "SmallNoise",
			"Type": "Float",
			"Default": 0.7,
			"Widget": "Slider",
			"Label": "Small Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "InsideNoise",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Label": "Inside Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "OutsideNoise",
			"Type": "Float",
			"Default": 0.5,
			"Widget": "Slider",
			"Label": "Outside Noise",
			"Constraints": [0.0, 1.0, 0.0, 0.0]
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


vec2 rotate(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float noise(vec2 uv)
{
	const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float fac = 0.16f;
	float f  = fac * 0.5000f * snoise( uv ); uv = m*uv;
	f += fac * 0.2500f * snoise( uv ); uv = m*uv;
	f += fac * 0.1250f * snoise( uv ); uv = m*uv;
	f += fac * 0.0625f * snoise( uv ); uv = m*uv;
	return f;
}


float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float rotation = 3.141f * u_Rotation / 180.0f;
	uv = 2.0f * uv - vec2(1.0f);
	uv = rotate(uv, rotation);
	float r = length(uv - u_Position) - snoise(uv) * u_LargeDistortion * 0.2f - noise(uv) * u_SmallDistortion;;
	float rad = u_Radius;
	float ns0 = u_Height * smoothstep(rad * (1 + u_OuterFalloff), rad, r) - u_Depth * smoothstep(rad, rad * u_InnerFalloff, r);
	float ns1 = snoise(uv) * u_LargeNoise * 0.2f + noise(uv) * u_SmallNoise;
	float ns2 = ns1 * ( u_InsideNoise * smoothstep(rad * 1.1f, rad * 0.9f, r) + u_OutsideNoise * smoothstep(rad * 0.9f, rad * 1.1f, r) ); 
	return (ns0 + ns2) * u_Strength;
}