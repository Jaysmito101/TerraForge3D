#version 430 core

layout(location = 0) out vec4 FragColor;

uniform mat4 u_InverseProjectionView;
uniform mat4 u_ProjectionView;
uniform vec3 u_CameraPosition;
uniform vec2 u_ViewportResolution;
uniform vec3 u_Color;
uniform sampler2D u_TerrainPlanarShadow;
uniform bool u_HasTerrainPlanarShadow;
uniform vec2 u_PlanarShadowMinimumXZ;
uniform vec2 u_PlanarShadowWorldSize;
uniform bool u_EnableSkyLight;
uniform samplerCube u_IrradianceMap;
uniform float u_SkyLightIntensity;
uniform vec3 u_SunDirection;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

const float INV_PI = 0.3183098861837907;

float SampleTerrainPlanarShadow(vec3 worldPosition)
{
	if (!u_HasTerrainPlanarShadow) return 1.0;
	vec2 shadowUv = (worldPosition.xz - u_PlanarShadowMinimumXZ) / u_PlanarShadowWorldSize;
	if (any(lessThan(shadowUv, vec2(0.0))) || any(greaterThan(shadowUv, vec2(1.0)))) return 1.0;
	return texture(u_TerrainPlanarShadow, shadowUv).r;
}

void main()
{
	vec2 ndc = (gl_FragCoord.xy / u_ViewportResolution) * 2.0 - 1.0;
	vec4 nearPoint = u_InverseProjectionView * vec4(ndc, -1.0, 1.0);
	vec4 farPoint = u_InverseProjectionView * vec4(ndc, 1.0, 1.0);
	nearPoint /= nearPoint.w;
	farPoint /= farPoint.w;

	vec3 rayOrigin = u_CameraPosition;
	vec3 rayDirection = normalize(farPoint.xyz - nearPoint.xyz);
	if (abs(rayDirection.y) < 0.000001) discard;

	float distanceToSurface = -rayOrigin.y / rayDirection.y;
	if (distanceToSurface <= 0.0) discard;

	vec3 surfacePosition = rayOrigin + rayDirection * distanceToSurface;
	vec4 projectedPosition = u_ProjectionView * vec4(surfacePosition, 1.0);
	float depth = projectedPosition.z / projectedPosition.w * 0.5 + 0.5;
	if (depth < 0.0 || depth > 1.0) discard;

	gl_FragDepth = depth;
	float terrainVisibility = SampleTerrainPlanarShadow(surfacePosition);
	vec3 lightDirection = normalize(-u_SunDirection);
	float nDotL = max(lightDirection.y, 0.0);
	vec3 directLighting = u_SunColor * u_SunIntensity * nDotL * terrainVisibility;
	vec3 skyLighting = u_EnableSkyLight
		? textureLod(u_IrradianceMap, vec3(0.0, 1.0, 0.0), 0.0).rgb * u_SkyLightIntensity
		: vec3(0.0);
	FragColor = vec4(u_Color * (directLighting + skyLighting) * INV_PI, 1.0);
}
