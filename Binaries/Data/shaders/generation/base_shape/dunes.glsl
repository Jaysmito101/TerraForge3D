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
	vec2 p = uv * u_Scale * 4.0f;
	vec2 n = floor( p );
    vec2 f = fract( p );

    float ns = 1.0;
    for( int j= -1; j <= 1; j++ ) for( int i=-1; i <= 1; i++ )
	{	
        vec2  g = vec2(i,j);
        vec2  o = random2( n + g );
        vec2  delta = g + o - f + vec2(snoise(uv * u_DistortionScale) * u_Distortion);
        float d = smoothstep(0.01f, 1.0f, length(delta));
        ns = smin(ns, d, u_Smoothness);
    }

	//float n = fabs(noise(val, nl.depth));
	return ns * u_Strength;
}