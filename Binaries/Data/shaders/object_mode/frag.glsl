#version 430 core

out vec4 FragColor;

in VertexData
{
  vec3 position;
  vec3 normal;
  vec4 texCoord;
} fragmentInput;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D u_Heightmap;

layout(std430, binding = 1) buffer SharedDataBuffer1
{
	vec4 sharedData1;
};

const float PI = 3.141592653589793;
const float INV_PI = 0.3183098861837907;
const float EPSILON = 0.0001;

const vec3 MATERIAL_ALBEDO = vec3(0.98, 0.96, 0.90);
const float MATERIAL_METALLIC = 0.0;
const float MATERIAL_ROUGHNESS = 0.58;

uniform int u_Resolution;
uniform float u_TileSize;
uniform bool u_InvertNormals;
uniform vec3 u_CameraPosition;

uniform vec3 u_SunDirection;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

uniform bool u_EnableSkyLight;
uniform float u_SkyLightIntensity;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_SpecularMap;
uniform sampler2D u_BrdfLut;

uniform bool u_IsViewportActive;
uniform vec2 u_MousePos;
uniform vec2 u_ViewportResolution;
uniform bool u_RequiresDrawBrush;
uniform vec4 u_BrushSettings0;
uniform vec3 u_MaskColor;
uniform bool u_DrawMask;
uniform bool u_InvertMask;
uniform sampler2D u_MaskTexture;

int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

#include "common/height_sampling.glsl"

vec3 calculateNormal()
{
	if (fragmentInput.texCoord.z > 0.5f)
	{
		vec3 solidNormal = normalize(fragmentInput.normal);
		return u_InvertNormals ? -solidNormal : solidNormal;
	}
	vec3 up = normalize(fragmentInput.normal);
	float height = SampleHeightBilinear(fragmentInput.texCoord.xy);
	vec3 basePosition = fragmentInput.position - up * height;
	vec2 heightGradient = SampleHeightGradient(fragmentInput.texCoord.xy);
	vec3 dx = dFdx(basePosition) + up * dot(heightGradient, dFdx(fragmentInput.texCoord.xy));
	vec3 dy = dFdy(basePosition) + up * dot(heightGradient, dFdy(fragmentInput.texCoord.xy));
	vec3 normal = normalize(cross(dx, dy));
	return u_InvertNormals ? -normal : normal;
}

float DistributionGGX(float nDotH, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSquared = alpha * alpha;
	float denominator = nDotH * nDotH * (alphaSquared - 1.0) + 1.0;
	return alphaSquared / max(PI * denominator * denominator, EPSILON);
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
	float k = ((roughness + 1.0) * (roughness + 1.0)) * 0.125;
	return nDotV / max(nDotV * (1.0 - k) + k, EPSILON);
}

float GeometrySmith(float nDotV, float nDotL, float roughness)
{
	return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 EvaluateSun(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
	vec3 L = normalize(-u_SunDirection);
	vec3 H = normalize(V + L);
	float nDotV = max(dot(N, V), 0.0);
	float nDotL = max(dot(N, L), 0.0);
	float nDotH = max(dot(N, H), 0.0);
	float vDotH = max(dot(V, H), 0.0);
	if (nDotL <= 0.0 || nDotV <= 0.0) return vec3(0.0);

	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F = FresnelSchlick(vDotH, F0);
	float D = DistributionGGX(nDotH, roughness);
	float G = GeometrySmith(nDotV, nDotL, roughness);
	vec3 specular = (D * G * F) / max(4.0 * nDotV * nDotL, EPSILON);
	vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo * INV_PI;
	vec3 radiance = u_SunColor * u_SunIntensity;
	return (diffuse + specular) * radiance * nDotL;
}

vec3 EvaluateImageBasedLighting(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
	if (!u_EnableSkyLight) return vec3(0.0);

	float nDotV = max(dot(N, V), 0.0);
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F = FresnelSchlickRoughness(nDotV, F0, roughness);
	vec3 kS = F;
	vec3 kD = (1.0 - kS) * (1.0 - metallic);

	// The irradiance precompute stores the integrated incoming diffuse light.
	vec3 irradiance = texture(u_IrradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo * kD * INV_PI;

	// The prefiltered cubemap contains the environment convolution for each
	// roughness mip. The BRDF LUT supplies the view/Fresnel integration term.
	vec3 reflectionDirection = reflect(-V, N);
	float maxSpecularLod = max(float(textureQueryLevels(u_SpecularMap) - 1), 0.0);
	vec3 prefilteredEnvironment = textureLod(u_SpecularMap, reflectionDirection, roughness * maxSpecularLod).rgb;
	vec2 brdf = texture(u_BrdfLut, vec2(nDotV, roughness)).rg;
	vec3 specular = prefilteredEnvironment * (F * brdf.x + brdf.y);
	return (diffuse + specular) * u_SkyLightIntensity;
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 ACESFilm(vec3 color)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main()
{
	if (u_IsViewportActive)
	{
		float distanceVal = length(gl_FragCoord.xy / u_ViewportResolution - u_MousePos);
		if (distanceVal < 1.0 / u_ViewportResolution.x)
		{
		sharedData1 = vec4(fragmentInput.texCoord.xy, 0.0, distanceVal);
		}
	}

	vec3 normal = calculateNormal();
	vec3 viewDirection = normalize(u_CameraPosition - fragmentInput.position);
	vec3 color = EvaluateSun(normal, viewDirection, MATERIAL_ALBEDO, MATERIAL_METALLIC, MATERIAL_ROUGHNESS);
		color += EvaluateImageBasedLighting(normal, viewDirection, MATERIAL_ALBEDO, MATERIAL_METALLIC, MATERIAL_ROUGHNESS);

	color = ACESFilm(max(color, vec3(0.0)));
	color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));

	if (u_DrawMask && fragmentInput.texCoord.z <= 0.5f)
	{
		float maskValue = texture(u_MaskTexture, fragmentInput.texCoord.xy).r;
		if (u_InvertMask) maskValue = 1.0 - maskValue;
		color = mix(color, u_MaskColor, clamp(maskValue * 0.65, 0.0, 1.0));
	}

	if (u_RequiresDrawBrush)
	{
		const vec3 brushColor = vec3(1.0, 0.0, 0.0);
		float distanceVal = length(fragmentInput.texCoord.xy - u_BrushSettings0.xy);
		float falloff = smoothstep(u_BrushSettings0.z * (1.0 - u_BrushSettings0.w), u_BrushSettings0.z, distanceVal);
		color = mix(color, brushColor, (1.0 - falloff) * 0.75);
	}

	FragColor = vec4(color, 1.0);
}
