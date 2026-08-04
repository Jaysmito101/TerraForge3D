#include "common/noise_2d.glsl"
#include "common/base_shape_helpers.glsl"

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
	float detail = tf3d_snoise2(domain.xy * 2.0f * noiseScale + vec2(domain.z)) * 0.06f * clamp(u_NoiseStrength, 0.0f, 4.0f);
	detail *= mix(0.35f, 1.0f, crackMask);
	return baseHeight + heightVariation + detail;
}
