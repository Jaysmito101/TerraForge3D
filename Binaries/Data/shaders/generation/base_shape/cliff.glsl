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

vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
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
	mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
	float f  = 0.5000f * snoise( uv ); uv = m*uv;
	f += 0.2500f * snoise( uv ); uv = m*uv;
	f += 0.1250f * snoise( uv ); uv = m*uv;
	f += 0.0625f * snoise( uv ); uv = m*uv;
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