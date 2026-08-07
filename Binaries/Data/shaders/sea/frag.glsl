#version 430 core

out vec4 FragColor;

uniform sampler2D u_HeightPyramid;
uniform int u_PyramidLevels;
uniform sampler2D u_SceneColor;
uniform sampler2D u_SceneDepth;
uniform samplerCube u_SpecularMap;

uniform mat4 u_InverseProjectionView;
uniform mat4 u_View;
uniform mat4 u_InverseProjection;
uniform bool u_Perspective;
uniform vec3 u_CameraPosition;
uniform vec2 u_ViewportResolution;

uniform vec2 u_TerrainMinimumXZ;
uniform vec2 u_TerrainWorldSize;
uniform float u_TerrainHeightOffset;
uniform float u_SeaWorldHeight;
uniform float u_BottomWorldHeight;
uniform float u_SideEdgeOffset;

uniform float u_Time;

uniform float u_WaveAmplitude; 
uniform float u_WaveFrequency; 
uniform float u_WaveSpeed;     

uniform float u_DetailStrength;

uniform vec3  u_ShallowColor;
uniform vec3  u_DeepColor;
uniform float u_DepthScale;     

uniform float u_ScatterStrength;

uniform float u_ReflectionStrength;
uniform float u_RefractionStrength;

uniform float u_SpecularIntensity;

uniform float u_FoamIntensity;
uniform float u_ShorefoamWidth;
uniform vec3  u_FoamColor;

uniform bool  u_EnableSkyLight;
uniform float u_SkyLightIntensity;
uniform vec3  u_SunDirection;
uniform vec3  u_SunColor;
uniform float u_SunIntensity;

const float WAVE_STEEPNESS     = 1.0;  
const float DETAIL_SCALE       = 31.4; 
const float FRESNEL_POWER      = 2.7;  
const float SPECULAR_TIGHTNESS = 256.0;
const float OPACITY            = 0.96; 
const float FOAM_SCALE         = 80.0; 
const float FOAM_COVERAGE      = 0.87; 
const float SHORE_WIDTH        = 0.055;
const float SHORE_SOFTNESS     = 0.0;  

#include "common/heightfield_pyramid_sampling.glsl"

const float PI  = 3.141592653589793;
const float TAU = 6.283185307179586;


float Hash21(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 Hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float GradientNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    vec2 du = 30.0 * f * f * (f * (f - 2.0) + 1.0);

    vec2 ga = Hash22(i + vec2(0.0, 0.0)) * 2.0 - 1.0;
    vec2 gb = Hash22(i + vec2(1.0, 0.0)) * 2.0 - 1.0;
    vec2 gc = Hash22(i + vec2(0.0, 1.0)) * 2.0 - 1.0;
    vec2 gd = Hash22(i + vec2(1.0, 1.0)) * 2.0 - 1.0;

    float va = dot(ga, f - vec2(0.0, 0.0));
    float vb = dot(gb, f - vec2(1.0, 0.0));
    float vc = dot(gc, f - vec2(0.0, 1.0));
    float vd = dot(gd, f - vec2(1.0, 1.0));

    return va + u.x * (vb - va) + u.y * (vc - va) + u.x * u.y * (va - vb - vc + vd);
}

mat2 FbmRotation() {
    const float angle = 1.618033 * PI; // golden angle
    float c = cos(angle), s = sin(angle);
    return mat2(c, s, -s, c);
}

float Fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float totalAmplitude = 0.0;
    mat2 rot = FbmRotation();

    for (int i = 0; i < octaves; i++) {
        value += amplitude * GradientNoise(p);
        totalAmplitude += amplitude;
        p = rot * p * 2.03 + vec2(17.1, 9.2);
        amplitude *= 0.49;
    }

    return value / max(totalAmplitude, 0.0001);
}

float RidgedNoise(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float totalAmplitude = 0.0;
    mat2 rot = FbmRotation();

    for (int i = 0; i < octaves; i++) {
        float n = 1.0 - abs(GradientNoise(p));
        value += amplitude * n * n;
        totalAmplitude += amplitude;
        p = rot * p * 2.07 + vec2(-11.7, 6.3);
        amplitude *= 0.48;
    }

    return value / max(totalAmplitude, 0.0001);
}

float WarpedNoise(vec2 p, vec2 flow) {
    vec2 q = vec2(
        Fbm(p * 0.6 + flow, 4),
        Fbm(p * 0.6 - flow + vec2(19.4, 7.1), 4)
    ) - 0.5;
    float base   = Fbm(p + q * 1.2, 5);
    float ridges = RidgedNoise(p * 1.5 - q * 0.4, 4);
    return base * 0.7 + ridges * 0.3;
}

struct WaterHit {
    bool  valid;
    bool  side;
    float rayDistance;
    vec3  worldPosition;
    vec2  terrainUv;
    float terrainHeight;
    vec3  normal;
};

struct GerstnerResult {
    float height;
    vec3  normal;
    float foam;   // crest-based foam factor
};

vec2 WorldToTerrainUv(vec2 worldXZ) {
    vec2 terrainMaxXZ = u_TerrainMinimumXZ + u_TerrainWorldSize;
    return vec2(
        (worldXZ.x - u_TerrainMinimumXZ.x) / max(u_TerrainWorldSize.x, 0.0001),
        (terrainMaxXZ.y - worldXZ.y) / max(u_TerrainWorldSize.y, 0.0001)
    );
}

bool IsInsideTerrain(vec2 uv) {
    return all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)));
}

vec3 ReconstructWorldPosition(vec2 screenUv, float depth) {
    vec4 clip = vec4(screenUv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 world = u_InverseProjectionView * clip;
    return world.xyz / max(abs(world.w), 1e-6);
}

vec3 GetRayDirection(vec2 screenUv) {
    vec2 ndc = screenUv * 2.0 - 1.0;
    vec4 nearPt = u_InverseProjectionView * vec4(ndc, -1.0, 1.0);
    vec4 farPt  = u_InverseProjectionView * vec4(ndc,  1.0, 1.0);
    nearPt /= max(abs(nearPt.w), 1e-6);
    farPt  /= max(abs(farPt.w),  1e-6);
    return normalize(farPt.xyz - nearPt.xyz);
}

float SampleSceneRayDistance(vec2 screenUv, vec3 rayOrigin, vec3 rayDir) {
    float d = texture(u_SceneDepth, clamp(screenUv, vec2(0.001), vec2(0.999))).r;
    if (d >= 0.999999) return -1.0;
    vec3 scenePos = ReconstructWorldPosition(screenUv, d);
    return dot(scenePos - rayOrigin, rayDir);
}

GerstnerResult EvaluateGerstnerWaves(vec2 worldXZ, float waterDepth) {
    const int NUM_WAVES = 8;
    const vec2 directions[8] = vec2[8](
        vec2(0.92, 0.38),   vec2(-0.38, 0.92),
        vec2(0.71, -0.71),  vec2(-0.87, -0.50),
        vec2(0.17, -0.99),  vec2(-0.62, 0.79),
        vec2(0.99,  0.10),  vec2(-0.26, -0.97)
    );
    const float freqMult[8]  = float[8](1.0, 1.32, 1.78, 2.35, 3.12, 4.07, 5.43, 7.18);
    const float ampMult[8]   = float[8](1.0, 0.62, 0.38, 0.22, 0.13, 0.075, 0.042, 0.024);
    const float speedMult[8] = float[8](1.0, 1.15, 0.87, 1.42, 0.73, 1.68, 0.92, 1.31);

    float depthAtten = smoothstep(0.0, max(u_WaveAmplitude * 3.0, 0.01), waterDepth);

    float totalHeight = 0.0;
    vec2  totalDisplaceXZ = vec2(0.0);
    vec2  totalGradient = vec2(0.0);
    float totalFoam = 0.0;
    float steepness = WAVE_STEEPNESS;

    for (int i = 0; i < NUM_WAVES; i++) {
        vec2 dir   = normalize(directions[i]);
        float freq = u_WaveFrequency * freqMult[i];
        float amp  = u_WaveAmplitude * ampMult[i] * depthAtten;
        float spd  = u_WaveSpeed * speedMult[i];
        float k    = TAU * freq;
        float omega = spd * k;

        float phase = dot(dir, worldXZ) * k - omega * u_Time;
        float s = sin(phase);
        float c = cos(phase);

        float Q = steepness / max(k * amp * float(NUM_WAVES), 0.001);
        Q = clamp(Q, 0.0, 1.0);

        totalDisplaceXZ += Q * amp * dir * c;
        totalHeight     += amp * s;
        totalGradient   += dir * (k * amp * c);

        float crestFactor = max(c, 0.0);
        totalFoam += crestFactor * crestFactor * ampMult[i];
    }

    vec3 normal = normalize(vec3(-totalGradient.x, 1.0, -totalGradient.y));

    GerstnerResult result;
    result.height = totalHeight;
    result.normal = normal;
    result.foam   = clamp(totalFoam / 2.5, 0.0, 1.0);
    return result;
}


vec3 ComputeDetailNormal(vec2 terrainUv, vec3 baseNormal) {
    float scale = DETAIL_SCALE;
    float strength = u_DetailStrength;

    vec2 flow1 = vec2(u_Time * 0.035, -u_Time * 0.023);
    vec2 flow2 = vec2(-u_Time * 0.028, u_Time * 0.019);

    float eps = 0.0008 / scale;
    vec2 p1 = terrainUv * scale + flow1;
    vec2 p2 = terrainUv * scale * 2.73 + flow2 + vec2(31.7, 17.3);

    float h1c = Fbm(p1, 5);
    float h1x = Fbm(p1 + vec2(eps, 0.0), 5);
    float h1z = Fbm(p1 + vec2(0.0, eps), 5);
    vec2 grad1 = vec2(h1x - h1c, h1z - h1c) / eps;

    float h2c = Fbm(p2, 4) * 0.5 + RidgedNoise(p2 * 0.8, 3) * 0.5;
    float h2x = Fbm(p2 + vec2(eps, 0.0), 4) * 0.5 + RidgedNoise((p2 + vec2(eps, 0.0)) * 0.8, 3) * 0.5;
    float h2z = Fbm(p2 + vec2(0.0, eps), 4) * 0.5 + RidgedNoise((p2 + vec2(0.0, eps)) * 0.8, 3) * 0.5;
    vec2 grad2 = vec2(h2x - h2c, h2z - h2c) / eps;

    vec2 p3 = terrainUv * scale * 7.5 + flow1 * 2.1 + vec2(53.2, 8.9);
    float h3c = GradientNoise(p3) * 0.5 + 0.5;
    float h3x = GradientNoise(p3 + vec2(eps * 2.0, 0.0)) * 0.5 + 0.5;
    float h3z = GradientNoise(p3 + vec2(0.0, eps * 2.0)) * 0.5 + 0.5;
    vec2 grad3 = vec2(h3x - h3c, h3z - h3c) / (eps * 2.0);

    vec2 combinedGrad = grad1 * 0.55 + grad2 * 0.32 + grad3 * 0.13;

    vec3 perturbation = vec3(
        -combinedGrad.x / max(u_TerrainWorldSize.x, 0.0001),
        0.0,
         combinedGrad.y / max(u_TerrainWorldSize.y, 0.0001)
    ) * strength;

    return normalize(baseNormal + perturbation);
}

float ComputeFoam(vec2 terrainUv, float waterDepth, float waveFoam) {
    float foam = 0.0;

    float shoreT = clamp(waterDepth / max(u_ShorefoamWidth, 0.001), 0.0, 1.0);
    if (shoreT < 1.0) {
        vec2 foamFlow1 = vec2(u_Time * 0.07, -u_Time * 0.048);
        vec2 foamFlow2 = vec2(-u_Time * 0.052, u_Time * 0.038);
        float noise1 = WarpedNoise(terrainUv * FOAM_SCALE * 0.35 + foamFlow1 * 0.4, foamFlow1 * 0.6);
        float noise2 = WarpedNoise(terrainUv * FOAM_SCALE * 0.62 + foamFlow2 * 0.3 + vec2(7.3, 13.1), foamFlow2 * 0.45);
        float combinedNoise = noise1 * 0.6 + noise2 * 0.4;

        float warpOffset1 = (combinedNoise - 0.5) * 2.2 + sin(terrainUv.x * 18.0 + u_Time * 0.3) * 0.15;
        float washPhase1 = shoreT * 4.0 - u_Time * 0.18 + warpOffset1;
        float wave1 = pow(max(0.5 + 0.5 * cos(washPhase1 * TAU), 0.0), 2.5);
        float retreatBias1 = smoothstep(0.0, 0.6, fract(washPhase1));
        wave1 *= mix(1.0, 0.4, retreatBias1);

        float warpOffset2 = (noise2 - 0.5) * 1.8 + cos(terrainUv.y * 22.0 - u_Time * 0.25) * 0.12;
        float washPhase2 = shoreT * 6.3 - u_Time * 0.24 + warpOffset2 + 1.92;
        float wave2 = pow(max(0.5 + 0.5 * cos(washPhase2 * TAU), 0.0), 3.5) * 0.55;

        float swellWarp = (noise1 - 0.5) * 1.0;
        float washPhase3 = shoreT * 2.0 - u_Time * 0.08 + swellWarp;
        float wave3 = pow(max(0.5 + 0.5 * cos(washPhase3 * TAU), 0.0), 2.0) * 0.35;

        float washFoam = max(max(wave1, wave2), wave3);

        float breakup = smoothstep(0.30, 0.68, combinedNoise);
        float bubbleCoord = terrainUv.x * FOAM_SCALE * 2.0 + terrainUv.y * FOAM_SCALE * 1.5;
        float bubbles = smoothstep(0.55, 0.85,
            GradientNoise(vec2(bubbleCoord, terrainUv.y * FOAM_SCALE * 3.0) + foamFlow1 * 2.5) * 0.5 + 0.5);

        float contactPulse = 0.7 + 0.3 * sin(u_Time * 1.2 + combinedNoise * TAU * 2.0);
        float contactFoam = (1.0 - smoothstep(0.0, 0.12, shoreT)) * 0.85 * contactPulse;
        contactFoam += bubbles * (1.0 - smoothstep(0.0, 0.25, shoreT)) * 0.3;

        float trailPhase = shoreT * 8.0 + u_Time * 0.15 + noise1 * 3.0;
        float trail = pow(max(0.5 + 0.5 * sin(trailPhase * TAU), 0.0), 6.0) * 0.25;
        trail *= smoothstep(0.05, 0.3, shoreT) * (1.0 - smoothstep(0.5, 0.9, shoreT));

        foam += ((washFoam * breakup) + contactFoam + trail) * (1.0 - shoreT * shoreT) * u_FoamIntensity;
    }

    float whitecapThreshold = 1.0 - FOAM_COVERAGE;
    float whitecaps = smoothstep(whitecapThreshold, whitecapThreshold + 0.25, waveFoam);

    vec2 wcFlow = vec2(u_Time * 0.05, -u_Time * 0.03);
    float wcNoise = WarpedNoise(terrainUv * FOAM_SCALE + wcFlow, wcFlow * 0.7);
    whitecaps *= smoothstep(0.3, 0.7, wcNoise);
    whitecaps *= smoothstep(0.0, 0.4, shoreT);

    foam += whitecaps * u_FoamIntensity * 0.7;

    return clamp(foam, 0.0, 1.0);
}


float ShoreBlendWidth() {
    float softnessWidth = u_DepthScale * mix(0.025, 0.55, SHORE_SOFTNESS);
    return max(max(SHORE_WIDTH, 0.0001), softnessWidth);
}

float HeightfieldWaterCoverage(vec2 terrainUv, float depthFeather) {
    float terrainHeight = TF3D_SamplePyramidChannel(u_HeightPyramid, terrainUv, 0, 2)
                        + u_TerrainHeightOffset;
    return smoothstep(-depthFeather, depthFeather, u_SeaWorldHeight - terrainHeight);
}

float SpatialShoreCoverage(vec2 terrainUv) {
    if (SHORE_SOFTNESS <= 0.0001) return 1.0;

    vec2 levelSize = vec2(textureSize(u_HeightPyramid, 0));
    vec2 texel = 1.0 / max(levelSize - vec2(1.0), vec2(1.0));
    float radius = mix(0.75, 12.0, SHORE_SOFTNESS);
    vec2 offsetX = vec2(texel.x * radius, 0.0);
    vec2 offsetY = vec2(0.0, texel.y * radius);
    float depthFeather = max(ShoreBlendWidth() * 0.12, 0.0001);

    float coverage  = HeightfieldWaterCoverage(terrainUv, depthFeather) * 0.20;
    coverage += HeightfieldWaterCoverage(terrainUv + offsetX, depthFeather) * 0.20;
    coverage += HeightfieldWaterCoverage(terrainUv - offsetX, depthFeather) * 0.20;
    coverage += HeightfieldWaterCoverage(terrainUv + offsetY, depthFeather) * 0.20;
    coverage += HeightfieldWaterCoverage(terrainUv - offsetY, depthFeather) * 0.20;
    return smoothstep(0.20, 0.98, coverage);
}


WaterHit EmptyWaterHit() {
    WaterHit hit;
    hit.valid = false;
    hit.side = false;
    hit.rayDistance = 0.0;
    hit.worldPosition = vec3(0.0);
    hit.terrainUv = vec2(0.0);
    hit.terrainHeight = 0.0;
    hit.normal = vec3(0.0, 1.0, 0.0);
    return hit;
}

WaterHit FindTopHit(vec3 rayOrigin, vec3 rayDir) {
    WaterHit hit = EmptyWaterHit();
    if (abs(rayDir.y) < 0.00001) return hit;

    float t = (u_SeaWorldHeight - rayOrigin.y) / rayDir.y;
    if (t <= 0.0) return hit;

    for (int iter = 0; iter < 3; iter++) {
        vec3 pos = rayOrigin + rayDir * t;
        vec2 uv = WorldToTerrainUv(pos.xz);
        if (!IsInsideTerrain(uv)) return EmptyWaterHit();

        float terrainH = TF3D_SamplePyramidChannel(u_HeightPyramid, clamp(uv, vec2(0.0), vec2(1.0)), 0, 2)
                        + u_TerrainHeightOffset;
        float depth = u_SeaWorldHeight - terrainH;
        if (depth <= 0.0001) return EmptyWaterHit();

        GerstnerResult waves = EvaluateGerstnerWaves(pos.xz, depth);
        float surfaceY = max(u_SeaWorldHeight + waves.height,
                             terrainH + max(depth * 0.7, 0.0005));
        t = (surfaceY - rayOrigin.y) / rayDir.y;
    }

    vec3 pos = rayOrigin + rayDir * t;
    vec2 uv = WorldToTerrainUv(pos.xz);
    if (!IsInsideTerrain(uv) || t <= 0.0) return EmptyWaterHit();

    float terrainH = TF3D_SamplePyramidChannel(u_HeightPyramid, clamp(uv, vec2(0.0), vec2(1.0)), 0, 2)
                    + u_TerrainHeightOffset;
    float depth = u_SeaWorldHeight - terrainH;
    if (depth <= 0.0001 || pos.y < terrainH) return EmptyWaterHit();

    GerstnerResult waves = EvaluateGerstnerWaves(pos.xz, depth);

    hit.valid = true;
    hit.side = false;
    hit.rayDistance = t;
    hit.worldPosition = pos;
    hit.terrainUv = uv;
    hit.terrainHeight = terrainH;
    hit.normal = waves.normal;
    return hit;
}

WaterHit FindSideHit(vec3 rayOrigin, vec3 rayDir) {
    WaterHit hit = EmptyWaterHit();
    vec2 safeDir = vec2(
        abs(rayDir.x) > 0.00001 ? rayDir.x : 0.00001,
        abs(rayDir.z) > 0.00001 ? rayDir.z : 0.00001
    );
    vec2 invDir = 1.0 / safeDir;
    vec2 t0 = (u_TerrainMinimumXZ - rayOrigin.xz) * invDir;
    vec2 t1 = (u_TerrainMinimumXZ + u_TerrainWorldSize - rayOrigin.xz) * invDir;
    vec2 tNear = min(t0, t1);
    vec2 tFar  = max(t0, t1);
    float nearDist = max(tNear.x, tNear.y);
    float farDist  = min(tFar.x, tFar.y);
    bool originInside = IsInsideTerrain(WorldToTerrainUv(rayOrigin.xz));
    float t = originInside ? farDist : nearDist;
    if (t <= 0.0 || farDist < nearDist) return hit;

    vec3 pos = rayOrigin + rayDir * t;
    vec2 uv = clamp(WorldToTerrainUv(pos.xz), vec2(0.0), vec2(1.0));
    float terrainH = TF3D_SamplePyramidChannel(u_HeightPyramid, uv, 0, 2) + u_TerrainHeightOffset;
    float depth = u_SeaWorldHeight - terrainH;
    if (depth <= 0.0001 ||
        pos.y < terrainH - u_SideEdgeOffset ||
        pos.y > u_SeaWorldHeight + u_SideEdgeOffset)
        return EmptyWaterHit();

    vec2 terrainMaxXZ = u_TerrainMinimumXZ + u_TerrainWorldSize;
    float edgeEps = max(max(u_TerrainWorldSize.x, u_TerrainWorldSize.y) * 0.0005, 0.0001);
    vec3 normal = vec3(0.0);
    if      (abs(pos.x - u_TerrainMinimumXZ.x) < edgeEps) normal = vec3(-1.0, 0.0, 0.0);
    else if (abs(pos.x - terrainMaxXZ.x)       < edgeEps) normal = vec3( 1.0, 0.0, 0.0);
    else if (abs(pos.z - u_TerrainMinimumXZ.y) < edgeEps) normal = vec3( 0.0, 0.0,-1.0);
    else                                                   normal = vec3( 0.0, 0.0, 1.0);

    hit.valid = true;
    hit.side = true;
    hit.rayDistance = t;
    hit.worldPosition = pos;
    hit.terrainUv = uv;
    hit.terrainHeight = terrainH;
    hit.normal = normal;
    return hit;
}


vec3 aces(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}


void main() {
    vec2 screenUv = gl_FragCoord.xy / max(u_ViewportResolution, vec2(1.0));
    vec3 rayOrigin = u_CameraPosition;
    vec3 rayDir = GetRayDirection(screenUv);

    WaterHit topHit  = FindTopHit(rayOrigin, rayDir);
    WaterHit sideHit = FindSideHit(rayOrigin, rayDir);
    WaterHit hit = topHit;
    if (sideHit.valid && (!hit.valid || sideHit.rayDistance < hit.rayDistance))
        hit = sideHit;
    if (!hit.valid) discard;

    float sceneRayDist = SampleSceneRayDistance(screenUv, rayOrigin, rayDir);
    if (sceneRayDist >= 0.0 && hit.rayDistance > sceneRayDist + 0.008)
        discard;

    float heightfieldDepth = max(u_SeaWorldHeight - hit.terrainHeight, 0.0);
    float rayWaterDepth = sceneRayDist >= 0.0
                        ? max(sceneRayDist - hit.rayDistance, 0.0)
                        : heightfieldDepth;
    float terrainIncidence = max(abs(dot(rayDir, hit.normal)), 0.15);
    float heightfieldRayDepth = heightfieldDepth / terrainIncidence;
    float waterDepth = hit.side ? heightfieldDepth : min(rayWaterDepth, heightfieldRayDepth);

    float shoreBlendW = ShoreBlendWidth();
    float shoreDepth = hit.side ? heightfieldDepth : min(waterDepth, heightfieldDepth);
    float shoreMaskW = max(shoreBlendW,
                           fwidth(shoreDepth) * mix(1.0, 2.5, SHORE_SOFTNESS));
    float depthShoreCoverage = smoothstep(0.0, shoreMaskW, shoreDepth);
    float spatialCoverage = hit.side ? 1.0 : SpatialShoreCoverage(hit.terrainUv);
    float shoreCoverage = mix(depthShoreCoverage,
                              min(depthShoreCoverage, spatialCoverage),
                              SHORE_SOFTNESS);

    vec3 normal = hit.normal;
    if (!hit.side) {
        normal = ComputeDetailNormal(hit.terrainUv, hit.normal);
    }

    vec3 viewDir = normalize(rayOrigin - hit.worldPosition);
    float NdotV = clamp(dot(normal, viewDir), 0.0, 1.0);
    vec3 lightDir = normalize(-u_SunDirection);
    float NdotL = max(dot(normal, lightDir), 0.0);

    float depthScale = max(u_DepthScale, 0.001);
    vec3 absorptionColor = mix(u_ShallowColor, u_DeepColor, 0.3) * 2.5 + vec3(0.05, 0.15, 0.12);
    absorptionColor = clamp(absorptionColor, 0.0, 1.0);
    vec3 absorption = vec3(
        exp(-waterDepth * (1.0 - absorptionColor.r) * 3.0 / depthScale),
        exp(-waterDepth * (1.0 - absorptionColor.g) * 3.0 / depthScale),
        exp(-waterDepth * (1.0 - absorptionColor.b) * 3.0 / depthScale)
    );
    float depthT = 1.0 - exp(-waterDepth / (depthScale * 0.4));
    depthT = clamp(depthT, 0.0, 1.0);
    vec3 waterColor = mix(u_ShallowColor, u_DeepColor, depthT);

    vec2 macroFlow = vec2(u_Time * 0.015, -u_Time * 0.011);
    float macroVar = WarpedNoise(hit.terrainUv * 1.2 + macroFlow, macroFlow * 0.4);
    waterColor *= mix(0.85, 1.10, macroVar);

    float bodyLighting = hit.side ? (0.20 + NdotL * 0.15) : (0.35 + NdotL * 0.30);
    vec3 waterBody = waterColor * bodyLighting;

    float sideVolume = hit.side
        ? clamp((u_SeaWorldHeight - hit.worldPosition.y) / max(heightfieldDepth, 0.001), 0.0, 1.0)
        : 0.0;
    if (hit.side) {
        waterBody *= mix(1.0, 0.30, sideVolume);
    }

    vec2 refractionOffset = hit.side ? vec2(0.0)
        : normal.xz * u_RefractionStrength * (0.3 + depthT * 0.4);
    vec2 refractedUv = clamp(screenUv + refractionOffset, vec2(0.001), vec2(0.999));
    vec3 refractedScene = pow(max(texture(u_SceneColor, refractedUv).rgb, vec3(0.0)), vec3(2.2));

    float refractionVis = 1.0;
    if (!hit.side) {
        vec3 refractRayDir = GetRayDirection(refractedUv);
        float refractSceneDist = SampleSceneRayDistance(refractedUv, rayOrigin, refractRayDir);
        if (refractSceneDist >= 0.0 && refractSceneDist < hit.rayDistance - 0.008)
            refractionVis = 0.0;
    }

    vec3 underwaterScene = refractedScene * absorption;

    float refractionBlend = hit.side ? 0.0
        : clamp((1.0 - depthT) * 0.35 * refractionVis, 0.0, 0.35);
    vec3 surface = mix(waterBody, underwaterScene, refractionBlend);

    float F0 = 0.02;
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, FRESNEL_POWER);
    fresnel = clamp(fresnel, 0.0, 1.0);

    if (u_EnableSkyLight && !hit.side) {
        vec3 reflDir = reflect(-viewDir, normal);
        float maxLod = max(float(textureQueryLevels(u_SpecularMap) - 1), 0.0);
        float roughness = mix(0.25, 0.08, depthT);
        vec3 envReflection = textureLod(u_SpecularMap, reflDir, roughness * maxLod).rgb;
        envReflection *= vec3(0.72, 0.85, 0.95);
        surface += envReflection * fresnel * u_ReflectionStrength * u_SkyLightIntensity * 0.5;
    }

    if (!hit.side) {
        vec3 halfVec = normalize(lightDir + viewDir);
        float NdotH = max(dot(normal, halfVec), 0.0);
        float specular = pow(NdotH, SPECULAR_TIGHTNESS);
        float specBroad = pow(NdotH, SPECULAR_TIGHTNESS * 0.15) * 0.06;
        surface += u_SunColor * u_SunIntensity * u_SpecularIntensity
                 * (specular + specBroad) * (0.04 + fresnel * 0.5);
    }

    if (!hit.side) {
        float sss = pow(max(dot(viewDir, -lightDir), 0.0), 4.0);
        GerstnerResult wavesForSSS = EvaluateGerstnerWaves(hit.worldPosition.xz, heightfieldDepth);
        float crestMask = clamp(wavesForSSS.height / max(u_WaveAmplitude * 0.5, 0.001), 0.0, 1.0);
        float shallowBoost = (1.0 - depthT) * 0.5;
        vec3 scatterCol = u_ShallowColor * 2.0 + vec3(0.01, 0.08, 0.06);
        surface += scatterCol * u_SunIntensity * sss * (crestMask + shallowBoost)
                 * u_ScatterStrength * 0.3;
    }

    GerstnerResult foamWaves = EvaluateGerstnerWaves(hit.worldPosition.xz, heightfieldDepth);
    float foam = hit.side ? 0.0 : ComputeFoam(hit.terrainUv, heightfieldDepth, foamWaves.foam);

    if (hit.side) {
        float landEdge = 1.0 - smoothstep(0.0, max(shoreBlendW * 1.8, 0.001),
                                           max(hit.worldPosition.y - hit.terrainHeight, 0.0));
        vec2 sideFlow = vec2(u_Time * 0.04, -u_Time * 0.03);
        float sideNoise = WarpedNoise(hit.terrainUv * FOAM_SCALE * 0.5 + sideFlow, sideFlow * 0.5);
        foam = landEdge * smoothstep(0.45, 0.85, sideNoise) * u_FoamIntensity * 0.35;
    }

    vec3 foamLit = u_FoamColor * (0.7 + NdotL * 0.3);
    surface = mix(surface, foamLit, foam);

    surface = aces(max(surface, vec3(0.0)));
    surface = pow(max(surface, vec3(0.0)), vec3(1.0 / 2.2));

    float alpha = hit.side
        ? OPACITY * mix(0.86, 0.98, sideVolume)
        : OPACITY * mix(0.82, 0.995, depthT);
    if (!hit.side) {
        float shoreFade = mix(1.0, shoreCoverage, SHORE_SOFTNESS);
        alpha *= shoreFade;
    }
    alpha = clamp(alpha, 0.0, 1.0);
    alpha = max(alpha, foam * 0.9);

    FragColor = vec4(surface, alpha);
}
