{
	"Name": "Cracks",
	"Params": [
		{
			"Name": "Strength",
			"Type": "Float",
			"Default": 0.58,
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},
		{
			"Name": "Scale",
			"Type": "Float",
			"Default": 1.4,
			"Widget": "Drag",
			"Sensitivity": 0.001,
			"Constraints": [0.001, 16.0, 0.0, 0.0]
		},
		{
			"Name": "CrackShapeDistortion",
			"Type": "Float",
			"Default": 0.380,
			"Widget": "Slider",
			"Constraints": [0.0, 0.45, 0.0, 0.0]
		},
		{
			"Name": "Smoothness",
			"Type": "Float",
			"Default": 0.12,
			"Widget": "Slider",
			"Constraints": [0.005, 1.0, 0.0, 0.0]
		},
		{
			"Name": "CrackWidth",
			"Label": "Crack Width",
			"Type": "Float",
			"Default": 0.12,
			"Widget": "Slider",
			"Constraints": [0.005, 0.5, 0.0, 0.0],
			"Tooltip": "Width of the softened crack floor. Larger values remove sharp cell seams."
		},
		{
			"Name": "RandomHeights",
			"Type": "Float",
			"Default": 2.8,
			"Widget": "Drag",
			"Sensitivity": 0.001,
			"Constraints": [0.0, 8.0, 0.0, 0.0]
		},		
		{
			"Name": "NoiseStrength",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.001,
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		},		
		{
			"Name": "NoiseScale",
			"Type": "Float",
			"Default": 1.0,
			"Widget": "Drag",
			"Sensitivity": 0.001,
			"Constraints": [0.001, 16.0, 0.0, 0.0]
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
		},
		{
			"Name": "MinMaxHeight",
			"Type": "Vector2",
			"Default": [0.1, 0.16],
			"Widget": "Drag",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 4.0, 0.0, 0.0]
		}
	]
}
// CODE

#include "common/base_shape_helpers.glsl"

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


float rand(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }
float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }
float rand(vec3 co){ return rand(co.xy+rand(co.z)); }

vec4 voronoi(vec3 sd)
{
	ivec3 cell = ivec3(floor(sd));
	vec3 nearestPoint = vec3(0.0f);
	float nearestDistance = 1.0e6f;
	float secondNearestDistance = 1.0e6f;
	float jitter = clamp(abs(u_CrackShapeDistortion), 0.0f, 0.45f);

	for (int i = -1 ; i <= 1 ; i++)
	{
		for (int j = -1 ; j <= 1 ; j++)
		{
			//int k = 0;
			for (int k = -1 ; k <= 1 ; k++)
			{
				vec3 neighbor = vec3(cell + ivec3(i, j, k));
				vec3 randomPoint = vec3(
					rand(neighbor + vec3(17.0f, 3.0f, 11.0f)),
					rand(neighbor + vec3(5.0f, 29.0f, 7.0f)),
					rand(neighbor + vec3(13.0f, 19.0f, 23.0f)));
				vec3 point = neighbor + vec3(0.5f) + (randomPoint - vec3(0.5f)) * (2.0f * jitter);
				vec3 sd3 = sd - point;
				float dst = dot(sd3, sd3);
				if(dst < nearestDistance)
				{
					secondNearestDistance = nearestDistance;
					nearestDistance = dst;
					nearestPoint = point;
				}
				else if (dst < secondNearestDistance)
				{
					secondNearestDistance = dst;
				}
			}
		}
	}

	float edgeDistance = sqrt(max(secondNearestDistance, 0.0f)) - sqrt(max(nearestDistance, 0.0f));
	return vec4(nearestPoint, clamp(edgeDistance, 0.0f, 1.0f));
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float scale = tf3d_shape_positive(u_Scale, 0.001f);
	vec3 offset = clamp(u_Offset, vec3(-10000.0f), vec3(10000.0f));
	vec3 domain = (seed + offset) * scale + vec3(u_Seed);
	vec4 voronoiResult = voronoi(domain);
	float edgeValue = clamp(voronoiResult.w, 0.0f, 1.0f);
	if(u_AbsoluteValue) edgeValue = abs(edgeValue);
	if(u_SquareValue) edgeValue = edgeValue * edgeValue;

	float strength = clamp(u_Strength, 0.0f, 4.0f);
	float smoothness = clamp(abs(u_Smoothness), 0.001f, 1.0f);
	float crackWidth = clamp(abs(u_CrackWidth), 0.005f, 0.5f);
	float lowHeight = clamp(min(u_MinMaxHeight.x, u_MinMaxHeight.y), 0.0f, 4.0f);
	float highHeight = clamp(max(u_MinMaxHeight.x, u_MinMaxHeight.y), lowHeight, 4.0f);
	float crackMask = tf3d_shape_smoothstep(0.0f, crackWidth, edgeValue);
	float cellHeight = edgeValue * strength;
	cellHeight = tf3d_shape_smax(cellHeight, lowHeight, smoothness);
	cellHeight = tf3d_shape_smin(cellHeight, highHeight, smoothness);
	float baseHeight = mix(lowHeight, cellHeight, crackMask);

	float randomHeight = rand(voronoiResult.xyz + vec3(u_Seed));
	float heightRange = max(highHeight - lowHeight, 0.0f);
	float heightVariation = (randomHeight - 0.5f) * heightRange
		* clamp(u_RandomHeights, 0.0f, 8.0f) * crackMask;

	float noiseScale = tf3d_shape_positive(u_NoiseScale, 0.001f);
	float detail = simplex3d(domain * 2.0f * noiseScale) * 0.06f * clamp(u_NoiseStrength, 0.0f, 4.0f);
	detail *= mix(0.35f, 1.0f, crackMask);
	return baseHeight + heightVariation + detail;
}
