{
	"Name": "Classic",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 1.84,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 0.04,
			"Widget": "Drag",
			"Sensitivity": 0.001
		},
		{
			"Name": "Levels",
			"Type": "Int",
			"Default": 2,
			"Widget": "Slider",
			"Constraints": [1.0, 6.0, 0.0, 0.0]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 42,
			"Widget": "Seed"
		},
		{
			"Name": "Offset",
			"Type": "Vector3",
			"Default": [0.0, 0.0, 0.0],
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "SquareValue",
			"Type": "Bool",
			"Default": false,
			"Widget": "Checkbox",
			"Label": "Square Value"
		},
		{
			"Name": "AbsoluteValue",
			"Type": "Bool",
			"Default": false,
			"Widget": "Checkbox",
			"Label": "Absolute Value"
		}
	]
}
// CODE

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) 
{
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333;
const float G3 =  0.1666667;

/* 3d simplex noise */
float simplex3d(vec3 p) 
{
	 /* 1. find current tetrahedron T and it's four vertices */
	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
	 
	 /* calculate s and x */
	 vec3 s = floor(p + dot(p, vec3(F3)));
	 vec3 x = p - s + dot(s, vec3(G3));
	 
	 /* calculate i1 and i2 */
	 vec3 e = step(vec3(0.0), x - x.yzx);
	 vec3 i1 = e*(1.0 - e.zxy);
	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	 /* x1, x2, x3 */
	 vec3 x1 = x - i1 + G3;
	 vec3 x2 = x - i2 + 2.0*G3;
	 vec3 x3 = x - 1.0 + 3.0*G3;
	 
	 /* 2. find four surflets and store them in d */
	 vec4 w, d;
	 
	 /* calculate surflet weights */
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);
	 
	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
	 w = max(0.6 - w, 0.0);
	 
	 /* calculate surflet components */
	 d.x = dot(random3(s), x);
	 d.y = dot(random3(s + i1), x1);
	 d.z = dot(random3(s + i2), x2);
	 d.w = dot(random3(s + 1.0), x3);
	 
	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 /* 3. return the sum of the four surflets */
	 return dot(d, vec4(52.0));
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	seed += u_Offset;
	float n = 0.0f;
	for(int i = 0 ; i < u_Levels ; i++)
	{
		seed = vec3(simplex3d(seed + vec3(0.0f, 0.0f, 0.0f)),
					simplex3d(seed + vec3(1.0f, 2.0f, 0.0f)),
					simplex3d(seed + vec3(3.0f, 4.0f, 0.0f))
					);
	}
	n = simplex3d(seed * u_Scale + vec3(u_Seed));
	if(u_AbsoluteValue) n = abs(n);
	if(u_SquareValue) n = n * n;	
	return n * u_Strength;
}