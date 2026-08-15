#version 430 core

out vec4 FragColor;

in VertexData
{
  vec3 position;
  vec3 basePosition;
  vec3 normal;
  vec4 texCoord;
} fragmentInput;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D u_Heightmap;

uniform sampler2D u_SlopeTexture;
uniform bool u_HasSlopeTexture;
uniform sampler2D u_TerrainSelfShadow;
uniform bool u_HasTerrainSelfShadow;
uniform sampler2D u_TerrainAmbient;
uniform bool u_HasTerrainAmbient;
uniform sampler2D u_TerrainGI;
uniform bool u_HasTerrainGI;

const float PI = 3.141592653589793;
const float INV_PI = 0.3183098861837907;
const float EPSILON = 0.0001;
const float UV_DETERMINANT_EPSILON = 1e-12;

const vec3 MATERIAL_ALBEDO = vec3(0.98, 0.96, 0.90);
const float MATERIAL_METALLIC = 0.0;
const float MATERIAL_ROUGHNESS = 0.58;

uniform int u_Resolution;
uniform float u_TileSize;
uniform bool u_InvertNormals;
uniform vec3 u_CameraPosition;
uniform bool u_ViewNormals;
uniform bool u_ViewSlope;
uniform bool u_ViewTerrainSelfShadow;
uniform bool u_ViewTerrainAmbient;
uniform bool u_ViewTerrainBentNormal;

uniform vec3 u_SunDirection;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;

uniform bool u_EnableSkyLight;
uniform float u_SkyLightIntensity;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_SpecularMap;
uniform sampler2D u_BrdfLut;

uniform bool u_RequiresDrawBrush;
uniform vec4 u_BrushSettings0;
uniform vec3 u_MaskColor;
uniform bool u_DrawMask;
uniform bool u_InvertMask;
uniform bool u_SignedMask;
uniform sampler2D u_MaskTexture;

int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

#include "common/height_sampling.glsl"
#include "common/heightfield_pyramid_sampling.glsl"

vec2 SampleTerrainGradient(vec2 texCoord)
{
	if (!u_HasSlopeTexture) return SampleHeightGradient(texCoord);

	vec2 texCoordDx = dFdx(texCoord);
	vec2 texCoordDy = dFdy(texCoord);
	vec2 sampledGradient = textureGrad(u_SlopeTexture, texCoord, texCoordDx, texCoordDy).rg;
	bool finiteGradient = all(equal(sampledGradient, sampledGradient)) &&
		all(lessThan(abs(sampledGradient), vec2(1000000.0)));
	bool hasGradientSignal = dot(abs(sampledGradient), vec2(1.0)) > 0.0000001;
	return finiteGradient && hasGradientSignal ? sampledGradient : SampleHeightGradient(texCoord);
}

vec3 calculateNormal()
{
	if (fragmentInput.texCoord.z > 0.5f)
	{
		vec3 solidNormal = normalize(fragmentInput.normal);
		return u_InvertNormals ? -solidNormal : solidNormal;
	}
	vec3 baseNormal = normalize(fragmentInput.normal);
	vec2 texCoord = fragmentInput.texCoord.xy;
	vec2 texCoordDx = dFdx(texCoord);
	vec2 texCoordDy = dFdy(texCoord);
	vec3 basePositionDx = dFdx(fragmentInput.basePosition);
	vec3 basePositionDy = dFdy(fragmentInput.basePosition);

	float uvDeterminant = texCoordDx.x * texCoordDy.y - texCoordDx.y * texCoordDy.x;
	if (abs(uvDeterminant) < UV_DETERMINANT_EPSILON)
	{
		vec3 fallbackCross = cross(basePositionDx, basePositionDy);
		vec3 fallbackNormal = length(fallbackCross) > EPSILON ? normalize(fallbackCross) : baseNormal;
		if (dot(fallbackNormal, baseNormal) < 0.0) fallbackNormal = -fallbackNormal;
		return u_InvertNormals ? -fallbackNormal : fallbackNormal;
	}
	vec3 baseTangentU = (basePositionDx * texCoordDy.y - basePositionDy * texCoordDx.y) / uvDeterminant;
	vec3 baseTangentV = (basePositionDy * texCoordDx.x - basePositionDx * texCoordDy.x) / uvDeterminant;

	vec2 heightGradient = SampleTerrainGradient(texCoord);
	vec3 terrainTangentU = baseTangentU + baseNormal * heightGradient.x;
	vec3 terrainTangentV = baseTangentV + baseNormal * heightGradient.y;
	vec3 normal = normalize(cross(terrainTangentU, terrainTangentV));
	if (dot(normal, baseNormal) < 0.0) normal = -normal;
	return u_InvertNormals ? -normal : normal;
}

float CalculateSpecularRoughness(vec3 normal)
{
	float normalVariance = 0.5 * (
		dot(dFdx(normal), dFdx(normal)) +
		dot(dFdy(normal), dFdy(normal)));
	const float specularAAStrength = 0.35;
	const float maxVarianceContribution = 0.20;
	float varianceContribution = min(max(normalVariance, 0.0) * specularAAStrength, maxVarianceContribution);
	return sqrt(clamp(MATERIAL_ROUGHNESS * MATERIAL_ROUGHNESS + varianceContribution, 0.0, 1.0));
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

float SampleTerrainSelfShadow()
{
	if (!u_HasTerrainSelfShadow || fragmentInput.texCoord.z > 0.5f) return 1.0;
	return texture(u_TerrainSelfShadow, clamp(fragmentInput.texCoord.xy, vec2(0.0), vec2(1.0))).r;
}

float SampleTerrainAmbientVisibility()
{
	if (!u_HasTerrainAmbient || fragmentInput.texCoord.z > 0.5f) return 1.0;
	return texture(u_TerrainAmbient, clamp(fragmentInput.texCoord.xy, vec2(0.0), vec2(1.0))).r;
}

vec3 SampleTerrainBentNormal(vec3 fallbackNormal)
{
	if (!u_HasTerrainAmbient || fragmentInput.texCoord.z > 0.5f) return fallbackNormal;
	return TF3D_OctahedralDecode(texture(u_TerrainAmbient, clamp(fragmentInput.texCoord.xy, vec2(0.0), vec2(1.0))).gb);
}

vec3 SampleTerrainGI()
{
	if (!u_HasTerrainGI || fragmentInput.texCoord.z > 0.5f) return vec3(0.0);
	return max(texture(u_TerrainGI, clamp(fragmentInput.texCoord.xy, vec2(0.0), vec2(1.0))).rgb, vec3(0.0));
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
	return (diffuse + specular) * radiance * nDotL * SampleTerrainSelfShadow();
}

vec3 EvaluateImageBasedLighting(vec3 N, vec3 diffuseNormal, vec3 V, vec3 albedo, float metallic, float roughness, float ambientVisibility)
{
	if (!u_EnableSkyLight) return vec3(0.0);

	float nDotV = max(dot(N, V), 0.0);
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F = FresnelSchlickRoughness(nDotV, F0, roughness);
	vec3 kS = F;
	vec3 kD = (1.0 - kS) * (1.0 - metallic);

	// The irradiance precompute stores the integrated incoming diffuse light.
	vec3 irradiance = textureLod(u_IrradianceMap, diffuseNormal, 0.0).rgb * ambientVisibility;
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
	vec3 normal = calculateNormal();
	if (u_ViewSlope)
	{
		vec2 gradient = SampleTerrainGradient(fragmentInput.texCoord.xy);
		FragColor = vec4(clamp(gradient * 0.035 + 0.5, 0.0, 1.0), 0.0, 1.0);
		return;
	}

	if (u_ViewNormals)
	{
		FragColor = vec4(normal * 0.5 + 0.5, 1.0);
		return;
	}

	if (u_ViewTerrainSelfShadow)
	{
		float visibility = SampleTerrainSelfShadow();
		FragColor = vec4(vec3(visibility), 1.0);
		return;
	}

	float ambientVisibility = SampleTerrainAmbientVisibility();
	vec3 bentNormal = SampleTerrainBentNormal(normal);
	if (u_ViewTerrainAmbient)
	{
		FragColor = vec4(vec3(ambientVisibility), 1.0);
		return;
	}
	if (u_ViewTerrainBentNormal)
	{
		FragColor = vec4(bentNormal * 0.5 + 0.5, 1.0);
		return;
	}
	
	float specularRoughness = CalculateSpecularRoughness(normal);
	vec3 viewDirection = normalize(u_CameraPosition - fragmentInput.position);
	vec3 color = EvaluateSun(normal, viewDirection, MATERIAL_ALBEDO, MATERIAL_METALLIC, specularRoughness);
	color += EvaluateImageBasedLighting(normal, bentNormal, viewDirection, MATERIAL_ALBEDO, MATERIAL_METALLIC, specularRoughness, ambientVisibility);
	color += SampleTerrainGI() * MATERIAL_ALBEDO * (1.0 - MATERIAL_METALLIC) * INV_PI;

	color = ACESFilm(max(color, vec3(0.0)));
	color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));

	if (u_DrawMask && fragmentInput.texCoord.z <= 0.5f)
	{
		float maskValue = texture(u_MaskTexture, fragmentInput.texCoord.xy).r;
		if (u_SignedMask)
		{
			if (u_InvertMask) maskValue = -maskValue;
			vec3 signedColor = maskValue < 0.0 ? vec3(0.08, 0.35, 1.0) : u_MaskColor;
			color = mix(color, signedColor, clamp(abs(maskValue) * 0.65, 0.0, 1.0));
		}
		else
		{
			if (u_InvertMask) maskValue = 1.0 - maskValue;
			color = mix(color, u_MaskColor, clamp(maskValue * 0.65, 0.0, 1.0));
		}
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
