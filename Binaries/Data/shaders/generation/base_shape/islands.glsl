{
	"Name": "Islands",
	"Params": [
		{
			"Name": "CoverageMode",
			"Type": "Int",
			"Default": 0,
			"Widget": "Dropdown",
			"Options": ["Single"]
		},
		{
			"Name": "Seed",
			"Type": "Int",
			"Default": 348,
			"Widget": "Seed"
		},
		{
			"Name": "TerrainOctaves",
			"Type": "Int",
			"Default": 5,
			"Widget": "Slider",
			"Constraints": [1.0, 16.0, 0.0, 0.0]
		},
		{
			"Name": "TerrainScale",
			"Label": "Terrain Scale",
			"Type": "Float",
			"Default": 8.0,
			"Widget": "Drag",
			"Sensitivity": 0.01
		},
		{
			"Name": "BeachCoverage",
			"Label": "Beach Coverage",
			"Type": "Float",
			"Default": 0.98,
			"Widget": "Slider",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 2.0, 0.0, 0.0]
		},
		{
			"Name": "BeachSteepness",
			"Label": "Beach Steepness",
			"Type": "Float",
			"Default": 0.887,
			"Widget": "Slider",
			"Sensitivity": 0.001,
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "BeachHeight",
			"Label": "Beach Height",
			"Type": "Float",
			"Default": 0.05,
			"Widget": "Drag",
			"Sensitivity": 0.001
		},
		{
			"Name": "ForestCoverage",
			"Label": "Forest Coverage",
			"Type": "Float",
			"Default": 0.8,
			"Widget": "Slider",
			"Sensitivity": 0.01,
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "ForestSteepness",
			"Label": "Forest Steepness",
			"Type": "Float",
			"Default": 0.887,
			"Widget": "Slider",
			"Sensitivity": 0.001,
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "ForestHeight",
			"Label": "Forest Height",
			"Type": "Float",
			"Default": 0.05,
			"Widget": "Drag",
			"Sensitivity": 0.001
		},
		{
			"Name": "MountainCoverage",
			"Label": "Mountain Coverage",
			"Type": "Float",
			"Default": 0.887,
			"Widget": "Slider",
			"Sensitivity": 0.001,
			"Constraints": [0.0, 1.0, 0.0, 0.0]
		},
		{
			"Name": "MountainHeight",
			"Label": "Mountain Height",
			"Type": "Float",
			"Default": 0.67,
			"Widget": "Drag",
			"Sensitivity": 0.001
		},
		{
			"Name": "Rotation",
			"Type": "Float",
			"Default": 0.0,
			"Widget": "Slider",
			"Constraints": [-180.0, 180.0, 0.0, 0.0]
		},
		{
			"Name": "Offset",
			"Type": "Vector2",
			"Default": [0.0, 0.0, 0.0],
			"Widget": "Drag",
			"Sensitivity": 0.01
		}
	]
}
// CODE


// Modified hash33 by Dave_Hoskins (original does not play well with simplex)
// Original Source: https://www.shadertoy.com/view/4djSRW
vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * vec3(0.1031, 0.11369, 0.13787));
    p3 += dot(p3, p3.yxz + 19.19);
    return -1.0 + 2.0 * fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}

// Raw simplex implementation by candycat
// Source: https://www.shadertoy.com/view/4sc3z2
float SimplexNoiseRaw(vec3 pos)
{
    const float K1 = 0.333333333;
    const float K2 = 0.166666667;
    
    vec3 i = floor(pos + (pos.x + pos.y + pos.z) * K1);
    vec3 d0 = pos - (i - (i.x + i.y + i.z) * K2);
    
    vec3 e = step(vec3(0.0), d0 - d0.yzx);
	vec3 i1 = e * (1.0 - e.zxy);
	vec3 i2 = 1.0 - e.zxy * (1.0 - e);
    
    vec3 d1 = d0 - (i1 - 1.0 * K2);
    vec3 d2 = d0 - (i2 - 2.0 * K2);
    vec3 d3 = d0 - (1.0 - 3.0 * K2);
    
    vec4 h = max(0.6 - vec4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
    vec4 n = h * h * h * h * vec4(dot(d0, hash33(i)), dot(d1, hash33(i + i1)), dot(d2, hash33(i + i2)), dot(d3, hash33(i + 1.0)));
    
    return dot(vec4(31.316), n);
}

float SimplexNoise(
    vec3  pos,
    float octaves,
    float scale,
    float persistence)
{
    float final        = 0.0;
    float amplitude    = 1.0;
    float maxAmplitude = 0.0;
    
    for(float i = 0.0; i < octaves; ++i)
    {
        final        += SimplexNoiseRaw(pos * scale) * amplitude;
        scale        *= 2.0;
        maxAmplitude += amplitude;
        amplitude    *= persistence;
    }
    
    return (final / maxAmplitude);
}

vec2 rotate(vec2 v, float a) 
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

float evaluateBaseShape(vec2 uv, vec3 seed)
{
	float rotation = 3.141f * u_Rotation / 180.0f;
	uv = uv * 2.0f - vec2(1.0) + u_Offset;
	uv += u_Offset;
	uv = rotate(uv, rotation);

	float beachMask = 1.0f, forestMask = 1.0f, mountainMask = 1.0f;
	float beachCoverageFactor = (1.0f - clamp(u_BeachCoverage, 0.0, 1.0));
	float forestCoverageFactor = beachCoverageFactor + (1.0f - beachCoverageFactor) * (1.0f - u_ForestCoverage);
	float mountainCoverageFactor = forestCoverageFactor +  (1.0f - forestCoverageFactor) * (1.0f - u_MountainCoverage);

	// if(u_CoverageMode == 0) // single coverage mode
	{
		float rad = length(uv) / u_BeachCoverage;
		float baseMask = exp(- rad * rad * 2.0f);
		beachMask = smoothstep(beachCoverageFactor, clamp(beachCoverageFactor + 1.0f - u_BeachSteepness * exp(-rad) * 0.9, beachCoverageFactor, 1.0f), baseMask);
		// forestMask = smoothstep(forestCoverageFactor, clamp(forestCoverageFactor + 1.0f - u_ForestSteepness * exp(-rad) * 0.9, forestCoverageFactor, 1.0f), baseMask);
	}
	
	float beachNoise = SimplexNoise(vec3(uv * u_TerrainScale * 0.1f, u_Seed), u_TerrainOctaves, 1.0f, 0.5f) * beachMask;
	float forestNoise = SimplexNoise(vec3(uv * u_TerrainScale * 0.12f, u_Seed), u_TerrainOctaves, 1.0f, 0.5f) * beachMask;
	float mountainNoise = SimplexNoise(vec3(uv * u_TerrainScale * 0.14f, u_Seed), u_TerrainOctaves, 1.0f, 0.5f);
	
	float beachResult = smoothstep(beachCoverageFactor, clamp(beachCoverageFactor + 1.0f - u_BeachSteepness, beachCoverageFactor, 1.0f), beachNoise);
	float forestResult = smoothstep(forestCoverageFactor, clamp(forestCoverageFactor + 1.0f - u_ForestSteepness, forestCoverageFactor, 1.0f), forestNoise);
	float mountainResult = smoothstep(mountainCoverageFactor, clamp(mountainCoverageFactor + 1.0f, mountainCoverageFactor, 1.0f), mountainNoise);
	
	return beachResult * u_BeachHeight + forestResult * u_ForestHeight + mountainResult * u_MountainHeight;
}
