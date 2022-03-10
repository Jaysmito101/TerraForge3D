#version 430 core
out vec4 FragColor;

#define NUM_TEXTURE_LAYERS 8

uniform vec3 _CameraPos;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;
uniform float _LightStrength;

uniform sampler2D _Albedo;
uniform sampler2D _AO;
uniform sampler2D _Metallic;
uniform sampler2D _Roughness;
uniform sampler2D _Normal;

in float height;
in float Distance;
in vec4 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in mat3 oTBN;

const float gamma = 2.2f;
const float PI = 3.14159265359;

vec3 linearToneMapping(vec3 color)
{
	float exposure = 1.;
	color = clamp(exposure * color, 0., 1.);
	color = pow(color, vec3(1. / gamma));
	return color;
}


// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

void main()
{	
	vec3 FragPosA = FragPos.xyz;
	vec3 color = vec3(1.0f);

	color = pow(texture(_Albedo, TexCoord).rgb, vec3(gamma));
	vec3 norm = texture(_Normal, TexCoord).rgb;
	norm = oTBN * (norm * 2.0f - 1.0f);
	vec3 albedo = color;
	float metallic = texture(_Metallic, TexCoord).r;
	float roughness = texture(_Roughness, TexCoord).r;
	float ao = texture(_AO, TexCoord).r;

	vec3 N = -norm;
	vec3 V = normalize(_CameraPos - FragPosA);

// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
        // calculate per-light radiance
        vec3 L = normalize(_LightPosition - FragPosA);
        vec3 H = normalize(V + L);
        float distance = length(_LightPosition - FragPosA);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = _LightColor * attenuation*_LightStrength;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0f/gamma)); 

	FragColor = vec4(color, 1.0f);
} 