#version 430 core

out vec4 FragColor;

in SeaVertexData
{
	vec3 worldPosition;
	vec3 waveNormal;
	vec2 texCoord;
	float waterDepth;
	float waveSteepness;
	float terrainCoverage;
} fragmentInput;

uniform sampler2D u_HeightPyramid;
uniform int u_PyramidLevels;
uniform sampler2D u_SceneColor;
uniform sampler2D u_SceneDepth;
uniform samplerCube u_SpecularMap;

uniform vec3 u_CameraPosition;
uniform vec2 u_ViewportResolution;
uniform mat4 u_View;
uniform mat4 u_InverseProjection;
uniform bool u_Perspective;
uniform vec2 u_TerrainWorldSize;
uniform float u_TerrainHeightOffset;
uniform float u_SeaWorldHeight;
uniform float u_SideEdgeOffset;
uniform bool u_VerticalSidePass;
uniform float u_DeepDepth;

uniform float u_Time;
uniform float u_NormalStrength;
uniform float u_NormalScale;
uniform float u_RefractionStrength;
uniform float u_ReflectionStrength;
uniform float u_Opacity;
uniform float u_FoamStrength;
uniform float u_FoamScale;
uniform float u_FoamSpeed;
uniform vec3 u_ShallowColor;
uniform vec3 u_DeepColor;
uniform vec3 u_FoamColor;

uniform bool u_EnableSkyLight;
uniform float u_SkyLightIntensity;
uniform vec3 u_SunDirection;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;
uniform float u_ShoreWidth;

#include "common/heightfield_pyramid_sampling.glsl"

const float TF3D_PI = 3.141592653589793;

float Hash12(vec2 value)
{
	return fract(sin(dot(value, vec2(127.1, 311.7))) * 43758.5453123);
}

float ValueNoise(vec2 value)
{
	vec2 cell = floor(value);
	vec2 local = fract(value);
	local = local * local * (3.0 - 2.0 * local);
	float bottom = mix(Hash12(cell), Hash12(cell + vec2(1.0, 0.0)), local.x);
	float top = mix(Hash12(cell + vec2(0.0, 1.0)), Hash12(cell + vec2(1.0, 1.0)), local.x);
	return mix(bottom, top, local.y);
}

float FoamNoise(vec2 worldXZ)
{
	vec2 motion = vec2(u_Time * u_FoamSpeed, -u_Time * u_FoamSpeed * 0.71);
	vec2 coordinate = fragmentInput.texCoord * u_FoamScale + worldXZ * 0.015;
	vec2 warp = vec2(
		ValueNoise(coordinate * 0.42 + motion * 0.7),
		ValueNoise(coordinate * 0.42 - motion * 0.53 + vec2(9.2, 3.7))) - 0.5;
	coordinate += warp * 2.2;
	float broad = ValueNoise(coordinate * 0.72 + motion);
	float detail = ValueNoise(coordinate * 1.83 - motion * 1.31);
	float broken = 1.0 - abs(ValueNoise(coordinate * 3.15 + motion * 0.35) * 2.0 - 1.0);
	return clamp(broad * 0.50 + detail * 0.28 + broken * 0.22, 0.0, 1.0);
}

float WaterSurfacePattern(vec2 worldXZ)
{
	vec2 motion = vec2(u_Time * 0.07, -u_Time * 0.047);
	vec2 coordinate = fragmentInput.texCoord * max(u_NormalScale, 0.01) + worldXZ * 0.015;
	vec2 warp = vec2(
		ValueNoise(coordinate * 0.48 + motion),
		ValueNoise(coordinate * 0.48 - motion + vec2(13.7, 5.4))) - 0.5;
	vec2 warped = coordinate + warp * 1.55;
	float broad = ValueNoise(warped * 0.62 + motion);
	float detail = ValueNoise(warped * 1.76 - motion * 1.6);
	float ridges = 1.0 - abs(ValueNoise(warped * 3.4 + motion * 0.31) * 2.0 - 1.0);
	return clamp(broad * 0.48 + detail * 0.29 + ridges * 0.23, 0.0, 1.0);
}

float MacroWaterPattern()
{
	vec2 motion = vec2(u_Time * 0.018, -u_Time * 0.013);
	vec2 coordinate = fragmentInput.texCoord * 1.35 + motion;
	vec2 warp = vec2(
		ValueNoise(coordinate * 0.72 + vec2(6.4, 11.2)),
		ValueNoise(coordinate * 0.72 + vec2(18.1, 2.7))) - 0.5;
	float broad = ValueNoise((coordinate + warp * 0.72) * 1.05);
	float basin = 1.0 - abs(ValueNoise((coordinate - warp * 0.38) * 1.92) * 2.0 - 1.0);
	return clamp(broad * 0.68 + basin * 0.32, 0.0, 1.0);
}

vec3 BuildWaterNormal()
{
	float epsilonValue = max(0.0015, 0.012 / max(u_NormalScale, 0.25));
	vec2 epsilon = vec2(epsilonValue);
	float center = WaterSurfacePattern(fragmentInput.worldPosition.xz);
	float xOffset = WaterSurfacePattern(fragmentInput.worldPosition.xz + vec2(epsilon.x, 0.0));
	float zOffset = WaterSurfacePattern(fragmentInput.worldPosition.xz + vec2(0.0, epsilon.y));
	vec2 noiseGradient = vec2((xOffset - center) / epsilon.x, (zOffset - center) / epsilon.y);
	return normalize(fragmentInput.waveNormal +
		vec3(-noiseGradient.x, 0.0, -noiseGradient.y) * u_NormalStrength * 0.010);
}

vec3 ACESFilm(vec3 color)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 ReconstructViewPosition(vec2 screenUv, float depth)
{
	vec4 clipPosition = vec4(screenUv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 viewPosition = u_InverseProjection * clipPosition;
	return viewPosition.xyz / max(abs(viewPosition.w), 0.000001);
}

float SampleSceneViewDepth(vec2 screenUv)
{
	float depth = texture(u_SceneDepth,
		clamp(screenUv, vec2(0.001), vec2(0.999))).r;
	if (depth >= 0.999999) return -1.0;
	return -ReconstructViewPosition(screenUv, depth).z;
}

float SampleViewRayWaterDepth(vec2 screenUv, vec3 worldPosition)
{
	float sceneDepth = texture(u_SceneDepth,
		clamp(screenUv, vec2(0.001), vec2(0.999))).r;
	if (sceneDepth >= 0.999999) return -1.0;

	vec3 sceneViewPosition = ReconstructViewPosition(screenUv, sceneDepth);
	vec3 waterViewPosition = (u_View * vec4(worldPosition, 1.0)).xyz;
	vec3 rayDirection = u_Perspective
		? normalize(waterViewPosition)
		: vec3(0.0, 0.0, -1.0);
	return dot(sceneViewPosition - waterViewPosition, rayDirection);
}

vec3 GetViewRayDirection(vec3 worldPosition)
{
	vec3 waterViewPosition = (u_View * vec4(worldPosition, 1.0)).xyz;
	return u_Perspective
		? normalize(waterViewPosition)
		: vec3(0.0, 0.0, -1.0);
}

void main()
{
	float waterDepth = fragmentInput.waterDepth;
	float sampledWaterDepth = waterDepth;
	float terrainHeight = 0.0;
	if (u_VerticalSidePass)
	{
		terrainHeight = TF3D_SamplePyramidChannel(
			u_HeightPyramid, fragmentInput.texCoord, 0, 2) + u_TerrainHeightOffset;
		sampledWaterDepth = u_SeaWorldHeight - terrainHeight;
		if (sampledWaterDepth <= 0.0001 ||
			fragmentInput.worldPosition.y < terrainHeight - u_SideEdgeOffset)
		{
			discard;
		}
		waterDepth = sampledWaterDepth;
	}
	else
	{
		terrainHeight = TF3D_SamplePyramidChannel(
			u_HeightPyramid, fragmentInput.texCoord, 0, 2) + u_TerrainHeightOffset;
		sampledWaterDepth = u_SeaWorldHeight - terrainHeight;
		waterDepth = fragmentInput.terrainCoverage > 0.5
			? sampledWaterDepth
			: max(sampledWaterDepth, u_DeepDepth * 2.0);
	}

	float heightfieldWaterDepth = max(sampledWaterDepth, 0.0);
	vec3 terrainNormal = vec3(0.0, 1.0, 0.0);
	if (!u_VerticalSidePass)
	{
		terrainNormal = TF3D_SamplePyramidTerrainNormal(
			u_HeightPyramid, fragmentInput.texCoord, u_TerrainWorldSize);
	}
	if (!u_VerticalSidePass)
	{
		float viewRayDepth = SampleViewRayWaterDepth(
			gl_FragCoord.xy / max(u_ViewportResolution, vec2(1.0)),
			fragmentInput.worldPosition);
		if (viewRayDepth >= 0.0)
		{
			vec3 viewRayDirection = GetViewRayDirection(fragmentInput.worldPosition);
			float terrainIncidence = max(abs(dot(viewRayDirection, terrainNormal)), 0.15);
			float heightfieldRayDepth = heightfieldWaterDepth / terrainIncidence;
			waterDepth = min(max(viewRayDepth, 0.0), heightfieldRayDepth);
		}
		else
			waterDepth = heightfieldWaterDepth;
	}

	vec3 normal = u_VerticalSidePass
		? normalize(fragmentInput.waveNormal)
		: BuildWaterNormal();
	vec3 viewDirection = normalize(u_CameraPosition - fragmentInput.worldPosition);
	float nDotV = clamp(dot(normal, viewDirection), 0.0, 1.0);
	float opticalDepth = max(waterDepth, 0.0);
	float depthFactor = 1.0 - exp(-opticalDepth /
		max(u_DeepDepth * 0.42, 0.001));
	depthFactor = clamp(depthFactor, 0.0, 1.0);
	float shallowFactor = exp(-heightfieldWaterDepth /
		max(u_DeepDepth * 0.34, 0.001));
	float shoreBand = 1.0 - smoothstep(0.0, max(u_ShoreWidth, 0.001),
		heightfieldWaterDepth);
	float shoreWideBand = 1.0 - smoothstep(0.0,
		max(u_ShoreWidth * 3.0, 0.001), heightfieldWaterDepth);

	float terrainSlope = 0.0;
	if (!u_VerticalSidePass)
	{
		terrainSlope = 1.0 - clamp(dot(terrainNormal, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
	}
	float surfacePattern = WaterSurfacePattern(fragmentInput.worldPosition.xz);
	float macroPattern = MacroWaterPattern();
	float foamPattern = FoamNoise(fragmentInput.worldPosition.xz);
	float brokenFoam = smoothstep(0.56, 0.82, foamPattern);
	float foam = shoreBand * brokenFoam * u_FoamStrength;
	foam *= 0.72 + terrainSlope * 0.48;
	if (u_VerticalSidePass)
	{
		float landEdgeBand = 1.0 - smoothstep(
			0.0, max(u_ShoreWidth * 1.8, 0.001),
			max(fragmentInput.worldPosition.y - terrainHeight, 0.0));
		foam = landEdgeBand * smoothstep(0.58, 0.84, foamPattern) *
			u_FoamStrength * 0.30;
	}
	float crestFoam = smoothstep(0.76, 0.96, fragmentInput.waveSteepness) *
		smoothstep(0.62, 0.88, surfacePattern) * u_FoamStrength * 0.08;
	foam = clamp(foam + crestFoam, 0.0, 1.0);

	vec2 screenUv = gl_FragCoord.xy / max(u_ViewportResolution, vec2(1.0));
	vec2 refractionOffset = u_VerticalSidePass
		? vec2(0.0)
		: normal.xz * u_RefractionStrength * (0.25 + depthFactor * 0.45);
	vec2 refractedUv = clamp(screenUv + refractionOffset, vec2(0.001), vec2(0.999));
	vec3 refractedScene = pow(max(texture(u_SceneColor, refractedUv).rgb, vec3(0.0)), vec3(2.2));
	float refractionVisibility = 1.0;
	if (!u_VerticalSidePass)
	{
		float waterViewDepth = -(u_View * vec4(fragmentInput.worldPosition, 1.0)).z;
		float refractedSceneDepth = SampleSceneViewDepth(refractedUv);
		if (refractedSceneDepth >= 0.0 &&
			refractedSceneDepth < waterViewDepth - 0.002)
			refractionVisibility = 0.0;
	}

	vec3 lightDirection = normalize(-u_SunDirection);
	float nDotL = max(dot(normal, lightDirection), 0.0);
	vec3 waterColor = mix(u_DeepColor, u_ShallowColor, shallowFactor);
	float combinedPattern = clamp(macroPattern * 0.72 + surfacePattern * 0.28, 0.0, 1.0);
	waterColor *= mix(0.68, 1.08, combinedPattern);
	waterColor *= mix(1.0, 0.48, shoreBand * 0.70);

	float sideVolume = 0.0;
	if (u_VerticalSidePass)
	{
		sideVolume = clamp((u_SeaWorldHeight - fragmentInput.worldPosition.y) /
			max(sampledWaterDepth, 0.001), 0.0, 1.0);
		waterColor *= mix(1.0, 0.34, sideVolume);
		waterColor *= mix(0.88, 1.04, surfacePattern);
	}

	float bodyLighting = u_VerticalSidePass
		? (0.18 + nDotL * 0.12)
		: (0.42 + nDotL * 0.22);
	vec3 waterBody = waterColor * bodyLighting;
	if (!u_VerticalSidePass)
		waterBody += u_ShallowColor * shoreWideBand * (0.035 + surfacePattern * 0.025);

	vec3 underwaterScene = refractedScene * mix(
		vec3(0.52, 0.70, 0.76), vec3(0.20, 0.36, 0.48), depthFactor);
	float refractionMix = u_VerticalSidePass
		? 0.0
		: clamp((0.035 + shallowFactor * 0.12) *
			refractionVisibility, 0.0, 0.16);
	vec3 surface = mix(waterBody, underwaterScene, refractionMix);

	float fresnel = 0.025 + 0.975 * pow(1.0 - nDotV, 5.0);
	if (u_EnableSkyLight && !u_VerticalSidePass)
	{
		vec3 reflectionDirection = reflect(-viewDirection, normal);
		float maxLod = max(float(textureQueryLevels(u_SpecularMap) - 1), 0.0);
		float roughness = mix(0.30, 0.12, 1.0 - depthFactor);
		vec3 reflection = textureLod(u_SpecularMap,
			reflectionDirection, roughness * maxLod).rgb;
		reflection *= vec3(0.62, 0.78, 0.90);
		surface += reflection * fresnel * u_ReflectionStrength *
			u_SkyLightIntensity * 0.42;
	}

	if (!u_VerticalSidePass)
	{
		float sunHighlight = pow(max(dot(reflect(-lightDirection, normal), viewDirection), 0.0), 180.0);
		surface += u_SunColor * u_SunIntensity * sunHighlight * (0.025 + fresnel * 0.22);
		float rippleGlint = smoothstep(0.78, 0.96, surfacePattern) *
			(0.18 + fresnel * 0.62);
		surface += u_SunColor * u_SunIntensity * rippleGlint * 0.008;
	}

	surface = mix(surface, u_FoamColor, foam * 0.72);
	surface = ACESFilm(max(surface, vec3(0.0)));
	surface = pow(max(surface, vec3(0.0)), vec3(1.0 / 2.2));

	float alpha = u_VerticalSidePass
		? u_Opacity * mix(0.86, 0.98, sideVolume)
		: u_Opacity * mix(0.86, 0.995, depthFactor);
	alpha = clamp(alpha, 0.0, 1.0);
	alpha = max(alpha, foam * 0.24);
	FragColor = vec4(surface, alpha);
}
