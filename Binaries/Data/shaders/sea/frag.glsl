#version 430 core

/// Based on:
/// * https://www.d5render.com/posts/realistic-ocean-waves-d5-render
/// * https://www.labri.fr/perso/gonzato/Articles/GONZATO_Wave_JVCS2000.pdf

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
uniform float u_WaveLength;
uniform float u_WaveSpeed;
uniform float u_WaveChoppiness;
uniform float u_ShoreWidth;
uniform float u_ShoreSoftness;
uniform float u_DeepDepth;
uniform float u_NormalStrength;
uniform float u_NormalScale;
uniform float u_RefractionStrength;
uniform float u_ReflectionStrength;
uniform float u_Opacity;
uniform float u_FoamStrength;
uniform float u_NearshoreFoamWidth;
uniform float u_OffshoreFoamStrength;
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

#include "common/heightfield_pyramid_sampling.glsl"

const float TF3D_PI = 3.141592653589793;

struct WaterHit {
  bool valid;
  bool side;
  float rayDistance;
  vec3 worldPosition;
  vec2 terrainUv;
  float terrainHeight;
  vec3 normal;
};

struct WaveSample {
  float height;
  vec2 gradient;
  float crest;
};

float Hash12(vec2 value) {
  return fract(sin(dot(value, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 GradientVector(vec2 cell) {
  float angle = Hash12(cell) * (TF3D_PI * 2.0);
  return vec2(cos(angle), sin(angle));
}

vec2 RotateNoiseDomain(vec2 value) {
  return vec2(value.x * 0.764 - value.y * 0.645,
              value.x * 0.645 + value.y * 0.764);
}

float GradientNoise(vec2 value) {
  vec2 cell = floor(value);
  vec2 local = fract(value);
  vec2 fade = local * local * local * (local * (local * 6.0 - 15.0) + 10.0);

  float lower =
      mix(dot(GradientVector(cell), local),
          dot(GradientVector(cell + vec2(1.0, 0.0)), local - vec2(1.0, 0.0)),
          fade.x);
  float upper =
      mix(dot(GradientVector(cell + vec2(0.0, 1.0)), local - vec2(0.0, 1.0)),
          dot(GradientVector(cell + vec2(1.0, 1.0)), local - vec2(1.0, 1.0)),
          fade.x);

  return clamp(0.5 + mix(lower, upper, fade.y) * 0.92, 0.0, 1.0);
}

float Fbm(vec2 value) {
  float result = 0.0;
  float amplitude = 0.5;
  float normalization = 0.0;

  for (int octave = 0; octave < 5; ++octave) {
    result += GradientNoise(value) * amplitude;
    normalization += amplitude;
    value = RotateNoiseDomain(value * 2.03) + vec2(17.1, 9.2);
    amplitude *= 0.5;
  }

  return result / max(normalization, 0.0001);
}

float RidgedFbm(vec2 value) {
  float result = 0.0;
  float amplitude = 0.5;
  float normalization = 0.0;

  for (int octave = 0; octave < 5; ++octave) {
    float ridge = 1.0 - abs(GradientNoise(value) * 2.0 - 1.0);
    result += ridge * ridge * amplitude;
    normalization += amplitude;
    value = RotateNoiseDomain(value * 2.07) - vec2(11.7, 6.3);
    amplitude *= 0.5;
  }

  return result / max(normalization, 0.0001);
}

float WarpedFbm(vec2 value, vec2 flow) {
  vec2 warp = vec2(Fbm(value * 0.58 + flow),
                   Fbm(value * 0.58 - flow + vec2(19.4, 7.1))) -
              0.5;
  float base = Fbm(value + warp * 1.35);
  float ridges = RidgedFbm(value * 1.72 - warp * 0.42);
  return clamp(base * 0.68 + ridges * 0.32, 0.0, 1.0);
}

float MacroNoise(vec2 terrainUv) {
  vec2 motion = vec2(u_Time * 0.018, -u_Time * 0.013);
  vec2 coordinate = terrainUv * 1.35 + motion;
  float broad = WarpedFbm(coordinate * 0.78, motion * 0.45 + vec2(6.4, 11.2));
  float basin = RidgedFbm(coordinate * 1.55 - motion * 0.22 + vec2(18.1, 2.7));
  return clamp(broad * 0.68 + basin * 0.32, 0.0, 1.0);
}

float SurfacePattern(vec2 terrainUv) {
  vec2 motion = vec2(u_Time * 0.07, -u_Time * 0.047);
  vec2 coordinate = terrainUv * max(u_NormalScale, 0.01) + motion;
  float broad = WarpedFbm(coordinate * 0.58, motion * 0.7 + vec2(13.7, 5.4));
  float detail = Fbm(coordinate * 1.22 - motion * 1.6 + vec2(4.1, 16.3));
  float ridges = RidgedFbm(coordinate * 2.15 + motion * 0.31 - vec2(7.2, 3.8));
  return clamp(broad * 0.56 + detail * 0.31 + ridges * 0.13, 0.0, 1.0);
}

float FoamPattern(vec2 terrainUv) {
  vec2 motion = vec2(u_Time * u_FoamSpeed, -u_Time * u_FoamSpeed * 0.71);
  vec2 coordinate = terrainUv * u_FoamScale + motion;
  float broad = WarpedFbm(coordinate * 0.52, motion * 0.7 + vec2(9.2, 3.7));
  float detail = Fbm(coordinate * 1.72 - motion * 1.31 + vec2(2.8, 14.6));
  float broken = RidgedFbm(coordinate * 3.05 + motion * 0.35 - vec2(5.6, 10.4));
  return clamp(broad * 0.50 + detail * 0.28 + broken * 0.22, 0.0, 1.0);
}

float WaveShape(vec2 terrainUv) {
  vec2 drift = vec2(u_Time * u_WaveSpeed * 0.11, -u_Time * u_WaveSpeed * 0.073);
  vec2 macroCoordinate = terrainUv * 0.92 + drift * 0.20;
  float macroShape = Fbm(macroCoordinate * 0.72 + vec2(23.1, 8.4));
  vec2 coordinate = terrainUv / max(u_WaveLength, 0.001);
  vec2 warp = vec2(Fbm(coordinate * 0.34 + drift + vec2(17.3, 4.1)),
                   Fbm(coordinate * 0.34 - drift + vec2(31.7, 12.6))) -
              0.5;
  vec2 warped = coordinate + warp * 1.45;
  float broad = Fbm(warped * 0.78 + drift);
  float detail = Fbm(warped * 2.15 - drift * 1.7 + vec2(5.2, 19.8));
  float ridges = RidgedFbm(warped * 3.35 + drift * 0.4 - vec2(8.6, 2.4));
  return clamp(macroShape * 0.28 + broad * 0.43 + detail * 0.17 + ridges * 0.12,
               0.0, 1.0);
}

float ShoreBlendWidth() {
  float softness = clamp(u_ShoreSoftness, 0.0, 1.0);
  float softnessWidth = u_DeepDepth * mix(0.025, 0.55, softness);
  return max(max(u_ShoreWidth, 0.0001), softnessWidth);
}

float HeightfieldWaterCoverage(vec2 terrainUv, float depthFeather) {
  float terrainHeight =
      TF3D_SamplePyramidChannel(u_HeightPyramid, terrainUv, 0, 2) +
      u_TerrainHeightOffset;
  return smoothstep(-depthFeather, depthFeather,
                    u_SeaWorldHeight - terrainHeight);
}

float SpatialShoreCoverage(vec2 terrainUv) {
  float softness = clamp(u_ShoreSoftness, 0.0, 1.0);
  if (softness <= 0.0001)
    return 1.0;

  vec2 levelSize = vec2(textureSize(u_HeightPyramid, 0));
  vec2 texel = 1.0 / max(levelSize - vec2(1.0), vec2(1.0));
  float radius = mix(0.75, 12.0, softness);
  vec2 offsetX = vec2(texel.x * radius, 0.0);
  vec2 offsetY = vec2(0.0, texel.y * radius);
  float depthFeather = max(ShoreBlendWidth() * 0.12, 0.0001);

  float coverage = HeightfieldWaterCoverage(terrainUv, depthFeather) * 0.20;
  coverage +=
      HeightfieldWaterCoverage(terrainUv + offsetX, depthFeather) * 0.20;
  coverage +=
      HeightfieldWaterCoverage(terrainUv - offsetX, depthFeather) * 0.20;
  coverage +=
      HeightfieldWaterCoverage(terrainUv + offsetY, depthFeather) * 0.20;
  coverage +=
      HeightfieldWaterCoverage(terrainUv - offsetY, depthFeather) * 0.20;
  return smoothstep(0.20, 0.98, coverage);
}

WaveSample EvaluateWaves(vec2 terrainUv, float waterDepth) {
  WaveSample result;
  result.height = 0.0;
  result.gradient = vec2(0.0);
  result.crest = 0.0;

  const vec2 directions[6] =
      vec2[6](vec2(0.96, 0.28), vec2(-0.42, 0.91), vec2(0.68, -0.73),
              vec2(-0.88, -0.46), vec2(0.17, -0.985), vec2(-0.72, 0.54));
  const float lengthScale[6] = float[6](1.0, 0.63, 0.38, 0.22, 0.31, 0.16);
  const float amplitudeScale[6] = float[6](1.0, 0.48, 0.25, 0.12, 0.16, 0.07);
  const float speedScale[6] = float[6](1.0, 1.27, 0.78, 1.64, 0.56, 1.91);

  float shoreWidth = ShoreBlendWidth();
  float normalizedDepth =
      clamp(waterDepth / max(shoreWidth * 2.0, 0.0001), 0.0, 1.0);
  float shoalFactor = 1.0 - smoothstep(0.05, 0.95, normalizedDepth);
  float shallowAttenuation =
      smoothstep(0.0, max(shoreWidth * 0.18, 0.0001), max(waterDepth, 0.0));
  float depthSafety =
      clamp(waterDepth / max(u_WaveAmplitude * 1.75, 0.0001), 0.0, 1.0);
  float attenuation = shallowAttenuation * mix(0.25, 1.0, depthSafety);
  float choppiness = clamp(u_WaveChoppiness, 0.0, 2.0);
  vec2 drift = vec2(u_Time * u_WaveSpeed * 0.11, -u_Time * u_WaveSpeed * 0.073);
  vec2 variationCoordinate = terrainUv * 1.35 + drift * 0.24;
  vec2 phaseWarp =
      vec2(GradientNoise(variationCoordinate * 1.8 + vec2(41.7, 13.1)),
           GradientNoise(variationCoordinate * 1.8 - drift + vec2(8.2, 27.4))) -
      0.5;
  float envelopeNoise = Fbm(variationCoordinate * 0.72 + vec2(19.6, 36.1));
  float amplitudeEnvelope = mix(0.70, 1.24, envelopeNoise);

  for (int index = 0; index < 6; ++index) {
    vec2 direction = normalize(directions[index]);
    float wavelength = max(
        u_WaveLength * lengthScale[index] * mix(1.0, 0.58, shoalFactor), 0.001);
    float waveNumber = 2.0 * TF3D_PI / wavelength;
    float amplitude = u_WaveAmplitude * amplitudeScale[index] * attenuation *
                      amplitudeEnvelope;
    float phase = dot(terrainUv, direction) * waveNumber -
                  u_Time * u_WaveSpeed * speedScale[index] +
                  dot(phaseWarp, direction) * (0.65 + choppiness * 0.50);
    float sine = sin(phase);
    float cosine = cos(phase);
    float harmonicSine = sin(phase * 2.0 + 1.17);
    float harmonicCosine = cos(phase * 2.0 + 1.17);
    result.height +=
        amplitude * (sine + harmonicSine * (0.10 + choppiness * 0.22));
    result.gradient +=
        direction * (amplitude * waveNumber *
                     (cosine + harmonicCosine * (0.20 + choppiness * 0.44)));
    result.crest += abs(cosine) * amplitudeScale[index];
  }

  float epsilon = max(u_WaveLength * 0.035, 0.002);
  float shapeCenter = WaveShape(terrainUv);
  float shapeX = WaveShape(terrainUv + vec2(epsilon, 0.0));
  float shapeZ = WaveShape(terrainUv + vec2(0.0, epsilon));
  float shapeAmplitude = u_WaveAmplitude * attenuation *
                         mix(0.32, 0.82, clamp(choppiness * 0.75, 0.0, 1.0));
  result.height += (shapeCenter - 0.5) * shapeAmplitude;
  result.gradient +=
      vec2((shapeX - shapeCenter) / epsilon, (shapeZ - shapeCenter) / epsilon) *
      shapeAmplitude;
  result.crest = clamp(result.crest / 2.25, 0.0, 1.0);
  return result;
}

vec2 WorldToTerrainUv(vec2 worldXZ) {
  vec2 terrainMaximumXZ = u_TerrainMinimumXZ + u_TerrainWorldSize;
  return vec2(
      (worldXZ.x - u_TerrainMinimumXZ.x) / max(u_TerrainWorldSize.x, 0.0001),
      (terrainMaximumXZ.y - worldXZ.y) / max(u_TerrainWorldSize.y, 0.0001));
}

bool IsInsideTerrain(vec2 terrainUv) {
  return all(greaterThanEqual(terrainUv, vec2(0.0))) &&
         all(lessThanEqual(terrainUv, vec2(1.0)));
}

vec3 ReconstructWorldPosition(vec2 screenUv, float depth) {
  vec4 clipPosition = vec4(screenUv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
  vec4 worldPosition = u_InverseProjectionView * clipPosition;
  return worldPosition.xyz / max(abs(worldPosition.w), 0.000001);
}

vec3 GetRayDirection(vec2 screenUv) {
  vec2 ndc = screenUv * 2.0 - 1.0;
  vec4 nearPoint = u_InverseProjectionView * vec4(ndc, -1.0, 1.0);
  vec4 farPoint = u_InverseProjectionView * vec4(ndc, 1.0, 1.0);
  nearPoint /= max(abs(nearPoint.w), 0.000001);
  farPoint /= max(abs(farPoint.w), 0.000001);
  return normalize(farPoint.xyz - nearPoint.xyz);
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

WaterHit FindTopHit(vec3 rayOrigin, vec3 rayDirection) {
  WaterHit hit = EmptyWaterHit();
  if (abs(rayDirection.y) < 0.00001)
    return hit;

  float rayDistance = (u_SeaWorldHeight - rayOrigin.y) / rayDirection.y;
  if (rayDistance <= 0.0)
    return hit;

  for (int iteration = 0; iteration < 2; ++iteration) {
    vec3 position = rayOrigin + rayDirection * rayDistance;
    vec2 terrainUv = WorldToTerrainUv(position.xz);
    if (!IsInsideTerrain(terrainUv))
      return EmptyWaterHit();

    float terrainHeight =
        TF3D_SamplePyramidChannel(
            u_HeightPyramid, clamp(terrainUv, vec2(0.0), vec2(1.0)), 0, 2) +
        u_TerrainHeightOffset;
    float heightfieldDepth = u_SeaWorldHeight - terrainHeight;
    if (heightfieldDepth <= 0.0001)
      return EmptyWaterHit();

    WaveSample waves = EvaluateWaves(terrainUv, heightfieldDepth);
    float surfaceHeight =
        max(u_SeaWorldHeight + waves.height,
            terrainHeight + max(heightfieldDepth * 0.70, 0.0005));
    rayDistance = (surfaceHeight - rayOrigin.y) / rayDirection.y;
  }

  vec3 position = rayOrigin + rayDirection * rayDistance;
  vec2 terrainUv = WorldToTerrainUv(position.xz);
  if (!IsInsideTerrain(terrainUv) || rayDistance <= 0.0)
    return EmptyWaterHit();

  float terrainHeight =
      TF3D_SamplePyramidChannel(u_HeightPyramid,
                                clamp(terrainUv, vec2(0.0), vec2(1.0)), 0, 2) +
      u_TerrainHeightOffset;
  float heightfieldDepth = u_SeaWorldHeight - terrainHeight;
  if (heightfieldDepth <= 0.0001 || position.y < terrainHeight)
    return EmptyWaterHit();

  WaveSample waves = EvaluateWaves(terrainUv, heightfieldDepth);
  vec3 normal =
      normalize(vec3(-waves.gradient.x / max(u_TerrainWorldSize.x, 0.0001), 1.0,
                     waves.gradient.y / max(u_TerrainWorldSize.y, 0.0001)));
  hit.valid = true;
  hit.side = false;
  hit.rayDistance = rayDistance;
  hit.worldPosition = position;
  hit.terrainUv = terrainUv;
  hit.terrainHeight = terrainHeight;
  hit.normal = normal;
  return hit;
}

WaterHit FindSideHit(vec3 rayOrigin, vec3 rayDirection) {
  WaterHit hit = EmptyWaterHit();
  vec2 safeDirection =
      vec2(abs(rayDirection.x) > 0.00001 ? rayDirection.x : 0.00001,
           abs(rayDirection.z) > 0.00001 ? rayDirection.z : 0.00001);
  vec2 inverseDirection = 1.0 / safeDirection;
  vec2 t0 = (u_TerrainMinimumXZ - rayOrigin.xz) * inverseDirection;
  vec2 t1 = (u_TerrainMinimumXZ + u_TerrainWorldSize - rayOrigin.xz) *
            inverseDirection;
  vec2 nearValues = min(t0, t1);
  vec2 farValues = max(t0, t1);
  float nearDistance = max(nearValues.x, nearValues.y);
  float farDistance = min(farValues.x, farValues.y);
  bool originInside = IsInsideTerrain(WorldToTerrainUv(rayOrigin.xz));
  float rayDistance = originInside ? farDistance : nearDistance;
  if (rayDistance <= 0.0 || farDistance < nearDistance)
    return hit;

  vec3 position = rayOrigin + rayDirection * rayDistance;
  vec2 terrainUv = clamp(WorldToTerrainUv(position.xz), vec2(0.0), vec2(1.0));
  float terrainHeight =
      TF3D_SamplePyramidChannel(u_HeightPyramid, terrainUv, 0, 2) +
      u_TerrainHeightOffset;
  float heightfieldDepth = u_SeaWorldHeight - terrainHeight;
  if (heightfieldDepth <= 0.0001 ||
      position.y < terrainHeight - u_SideEdgeOffset ||
      position.y > u_SeaWorldHeight + u_SideEdgeOffset)
    return EmptyWaterHit();

  vec2 terrainMaximumXZ = u_TerrainMinimumXZ + u_TerrainWorldSize;
  float edgeEpsilon =
      max(max(u_TerrainWorldSize.x, u_TerrainWorldSize.y) * 0.0005, 0.0001);
  vec3 normal = vec3(0.0);
  if (abs(position.x - u_TerrainMinimumXZ.x) < edgeEpsilon)
    normal = vec3(-1.0, 0.0, 0.0);
  else if (abs(position.x - terrainMaximumXZ.x) < edgeEpsilon)
    normal = vec3(1.0, 0.0, 0.0);
  else if (abs(position.z - u_TerrainMinimumXZ.y) < edgeEpsilon)
    normal = vec3(0.0, 0.0, -1.0);
  else
    normal = vec3(0.0, 0.0, 1.0);

  hit.valid = true;
  hit.side = true;
  hit.rayDistance = rayDistance;
  hit.worldPosition = position;
  hit.terrainUv = terrainUv;
  hit.terrainHeight = terrainHeight;
  hit.normal = normal;
  return hit;
}

float SampleSceneRayDistance(vec2 screenUv, vec3 rayOrigin, vec3 rayDirection) {
  float sceneDepth =
      texture(u_SceneDepth, clamp(screenUv, vec2(0.001), vec2(0.999))).r;
  if (sceneDepth >= 0.999999)
    return -1.0;
  vec3 scenePosition = ReconstructWorldPosition(screenUv, sceneDepth);
  return dot(scenePosition - rayOrigin, rayDirection);
}

vec3 BuildSurfaceNormal(vec2 terrainUv, vec3 waveNormal) {
  float epsilon = max(0.0015, 0.012 / max(u_NormalScale, 0.25));
  float center = SurfacePattern(terrainUv);
  float xOffset = SurfacePattern(terrainUv + vec2(epsilon, 0.0));
  float zOffset = SurfacePattern(terrainUv + vec2(0.0, epsilon));
  vec2 gradient =
      vec2((xOffset - center) / epsilon, (zOffset - center) / epsilon);
  return normalize(waveNormal +
                   vec3(-gradient.x / max(u_TerrainWorldSize.x, 0.0001), 0.0,
                        gradient.y / max(u_TerrainWorldSize.y, 0.0001)) *
                       u_NormalStrength * 0.65);
}

float ACESChannel(float color) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0,
               1.0);
}

vec3 ACESFilm(vec3 color) {
  return vec3(ACESChannel(color.r), ACESChannel(color.g), ACESChannel(color.b));
}

void main() {
  vec2 screenUv = gl_FragCoord.xy / max(u_ViewportResolution, vec2(1.0));
  vec3 rayOrigin = u_CameraPosition;
  vec3 rayDirection = GetRayDirection(screenUv);

  WaterHit topHit = FindTopHit(rayOrigin, rayDirection);
  WaterHit sideHit = FindSideHit(rayOrigin, rayDirection);
  WaterHit hit = topHit;
  if (sideHit.valid && (!hit.valid || sideHit.rayDistance < hit.rayDistance))
    hit = sideHit;
  if (!hit.valid)
    discard;

  float sceneRayDistance =
      SampleSceneRayDistance(screenUv, rayOrigin, rayDirection);
  if (sceneRayDistance >= 0.0 && hit.rayDistance > sceneRayDistance + 0.008)
    discard;

  float heightfieldDepth = max(u_SeaWorldHeight - hit.terrainHeight, 0.0);
  float rayWaterDepth = sceneRayDistance >= 0.0
                            ? max(sceneRayDistance - hit.rayDistance, 0.0)
                            : heightfieldDepth;
  float terrainIncidence = max(abs(dot(rayDirection, hit.normal)), 0.15);
  float heightfieldRayDepth = heightfieldDepth / terrainIncidence;
  float waterDepth =
      hit.side ? heightfieldDepth : min(rayWaterDepth, heightfieldRayDepth);

  float depthFactor = 1.0 - exp(-waterDepth / max(u_DeepDepth * 0.42, 0.001));
  depthFactor = clamp(depthFactor, 0.0, 1.0);
  float shallowFactor = exp(-heightfieldDepth / max(u_DeepDepth * 0.34, 0.001));
  float shoreBlendWidth = ShoreBlendWidth();
  float shoreDepth =
      hit.side ? heightfieldDepth : min(waterDepth, heightfieldDepth);
  float shoreMaskWidth =
      max(shoreBlendWidth,
          fwidth(shoreDepth) * mix(1.0, 2.5, clamp(u_ShoreSoftness, 0.0, 1.0)));
  float shoreSoftness = clamp(u_ShoreSoftness, 0.0, 1.0);
  float depthShoreCoverage = smoothstep(0.0, shoreMaskWidth, shoreDepth);
  float spatialShoreCoverage =
      hit.side ? 1.0 : SpatialShoreCoverage(hit.terrainUv);
  float shoreCoverage =
      mix(depthShoreCoverage, min(depthShoreCoverage, spatialShoreCoverage),
          shoreSoftness);
  float shoreBand = 1.0 - shoreCoverage;
  float shoreWideBand =
      1.0 - smoothstep(0.0,
                       shoreMaskWidth *
                           mix(2.2, 3.8, clamp(u_ShoreSoftness, 0.0, 1.0)),
                       shoreDepth);
  shoreWideBand =
      max(shoreWideBand, (1.0 - spatialShoreCoverage) * shoreSoftness * 0.75);

  vec3 normal = hit.normal;
  if (!hit.side) {
    normal = BuildSurfaceNormal(hit.terrainUv, hit.normal);
  }
  vec3 viewDirection = normalize(rayOrigin - hit.worldPosition);
  float nDotV = clamp(dot(normal, viewDirection), 0.0, 1.0);
  float surfacePattern = SurfacePattern(hit.terrainUv);
  float macroPattern = MacroNoise(hit.terrainUv);
  float foamPattern = FoamPattern(hit.terrainUv);

  float terrainSlope = 0.0;
  if (!hit.side) {
    vec3 terrainNormal = TF3D_SamplePyramidTerrainNormal(
        u_HeightPyramid, hit.terrainUv, u_TerrainWorldSize);
    terrainSlope =
        1.0 - clamp(dot(terrainNormal, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
  }

  float patternSoftness = clamp(u_ShoreSoftness * 0.65, 0.0, 1.0);
  float shoreDepthT = clamp(shoreDepth / max(shoreMaskWidth, 0.0001), 0.0, 1.0);
  float contactWidthT = mix(0.045, 0.16, shoreSoftness);
  float contactBand = 1.0 - smoothstep(0.0, contactWidthT, shoreDepthT);
  contactBand = max(contactBand, (1.0 - spatialShoreCoverage) * shoreSoftness);
  float surfZone = smoothstep(0.04, 0.18, shoreDepthT) *
                   (1.0 - smoothstep(0.52, 0.95, shoreDepthT));

  float frontFrequency = mix(1.25, 3.4, clamp(u_FoamScale / 14.0, 0.0, 1.0));
  float frontWarp = (foamPattern - 0.5) * 3.4 + (macroPattern - 0.5) * 1.4 +
                    (surfacePattern - 0.5) * 0.8;
  float frontPhase = shoreDepthT * frontFrequency -
                     u_Time * u_FoamSpeed * 0.72 + frontWarp +
                     dot(hit.terrainUv, vec2(2.3, -1.7));
  float frontPhaseSecondary = shoreDepthT * frontFrequency * 1.63 -
                              u_Time * u_FoamSpeed * 0.91 - frontWarp * 0.72 +
                              dot(hit.terrainUv, vec2(-1.1, 3.7));
  float crestPower = mix(5.5, 1.9, clamp(u_NearshoreFoamWidth, 0.02, 0.8));
  float frontCrest =
      max(pow(0.5 + 0.5 * cos(frontPhase * TF3D_PI * 2.0), crestPower),
          pow(0.5 + 0.5 * cos(frontPhaseSecondary * TF3D_PI * 2.0),
              crestPower * 0.78) *
              0.72);
  float breakupField =
      clamp(foamPattern * 0.58 + macroPattern * 0.27 + surfacePattern * 0.15,
            0.0, 1.0);
  float frontBreakup = smoothstep(0.48, 0.74, breakupField);
  float breakingFront =
      surfZone * smoothstep(0.40, 0.70, frontCrest) * frontBreakup;
  float washFoam = surfZone * smoothstep(0.64, 0.88, foamPattern) * 0.10;
  float foam = 0.0;
  float crashFoam = 0.0;
  if (hit.side) {
    float landEdgeBand =
        1.0 - smoothstep(0.0, max(shoreBlendWidth * 1.8, 0.001),
                         max(hit.worldPosition.y - hit.terrainHeight, 0.0));
    foam = landEdgeBand *
           smoothstep(mix(0.58, 0.44, patternSoftness),
                      mix(0.84, 0.95, patternSoftness), foamPattern) *
           u_FoamStrength * 0.30;
  } else {
    float contactBreakup = smoothstep(mix(0.50, 0.38, patternSoftness),
                                      mix(0.80, 0.68, patternSoftness),
                                      foamPattern * 0.65 + macroPattern * 0.35);
    float contactWash = contactBand * contactBreakup * 0.46;
    float nearshoreFoam = contactWash + breakingFront + washFoam;
    float crashPulse = 0.72 + 0.28 * sin(u_Time * u_FoamSpeed * 2.4 +
                                         macroPattern * TF3D_PI * 5.0 +
                                         dot(hit.terrainUv, vec2(9.7, -7.3)));
    float foamResponse = 1.0 - exp(-max(u_FoamStrength, 0.0) * 2.8);
    crashFoam = nearshoreFoam * mix(0.72, 1.30, terrainSlope) * crashPulse *
                foamResponse;
    crashFoam = clamp(crashFoam, 0.0, 1.0);
    foam += crashFoam * (1.0 - foam);
  }
  float offshoreMask = hit.side ? 0.0 : smoothstep(0.34, 0.78, shoreDepthT);
  float rippleFoam = smoothstep(0.76, 0.96, 0.35 + surfacePattern * 0.65) *
                     smoothstep(0.62, 0.88, surfacePattern) *
                     u_OffshoreFoamStrength * offshoreMask;
  foam += rippleFoam;
  foam *= 0.72 + terrainSlope * 0.28;
  foam = clamp(foam, 0.0, 1.0);

  vec2 refractionOffset =
      hit.side ? vec2(0.0)
               : normal.xz * u_RefractionStrength * (0.25 + depthFactor * 0.45);
  vec2 refractedUv =
      clamp(screenUv + refractionOffset, vec2(0.001), vec2(0.999));
  vec3 refractedScene =
      pow(max(texture(u_SceneColor, refractedUv).rgb, vec3(0.0)), vec3(2.2));
  float refractionVisibility = 1.0;
  if (!hit.side) {
    vec3 refractedRayDirection = GetRayDirection(refractedUv);
    float refractedSceneDistance =
        SampleSceneRayDistance(refractedUv, rayOrigin, refractedRayDirection);
    if (refractedSceneDistance >= 0.0 &&
        refractedSceneDistance < hit.rayDistance - 0.008)
      refractionVisibility = 0.0;
  }

  float combinedPattern =
      clamp(macroPattern * 0.72 + surfacePattern * 0.28, 0.0, 1.0);
  vec3 waterColor = mix(u_DeepColor, u_ShallowColor, shallowFactor);
  waterColor *= mix(0.68, 1.08, combinedPattern);
  waterColor *= mix(1.0, 0.48, shoreBand * 0.70);

  float sideVolume = hit.side ? clamp((u_SeaWorldHeight - hit.worldPosition.y) /
                                          max(heightfieldDepth, 0.001),
                                      0.0, 1.0)
                              : 0.0;
  if (hit.side) {
    waterColor *= mix(1.0, 0.34, sideVolume);
    waterColor *= mix(0.88, 1.04, surfacePattern);
  }

  vec3 lightDirection = normalize(-u_SunDirection);
  float nDotL = max(dot(normal, lightDirection), 0.0);
  float bodyLighting = hit.side ? (0.18 + nDotL * 0.12) : (0.42 + nDotL * 0.22);
  vec3 waterBody = waterColor * bodyLighting;
  if (!hit.side)
    waterBody +=
        u_ShallowColor * shoreWideBand * (0.035 + surfacePattern * 0.025);

  vec3 underwaterScene =
      refractedScene *
      mix(vec3(0.52, 0.70, 0.76), vec3(0.20, 0.36, 0.48), depthFactor);
  float refractionMix =
      hit.side ? 0.0
               : clamp((0.035 + shallowFactor * 0.12) * refractionVisibility,
                       0.0, 0.16);
  vec3 surface = mix(waterBody, underwaterScene, refractionMix);

  float fresnel = 0.025 + 0.975 * pow(1.0 - nDotV, 5.0);
  if (u_EnableSkyLight && !hit.side) {
    vec3 reflectionDirection = reflect(-viewDirection, normal);
    float maxLod = max(float(textureQueryLevels(u_SpecularMap) - 1), 0.0);
    float roughness = mix(0.30, 0.12, 1.0 - depthFactor);
    vec3 reflection =
        textureLod(u_SpecularMap, reflectionDirection, roughness * maxLod).rgb;
    reflection *= vec3(0.62, 0.78, 0.90);
    surface += reflection * fresnel * u_ReflectionStrength *
               u_SkyLightIntensity * 0.42;
  }

  if (!hit.side) {
    float sunHighlight = pow(
        max(dot(reflect(-lightDirection, normal), viewDirection), 0.0), 180.0);
    surface +=
        u_SunColor * u_SunIntensity * sunHighlight * (0.025 + fresnel * 0.22);
    float rippleGlint =
        smoothstep(0.78, 0.96, surfacePattern) * (0.18 + fresnel * 0.62);
    surface += u_SunColor * u_SunIntensity * rippleGlint * 0.008;
  }

  float foamColorMix = clamp(foam * 0.72 + crashFoam * 0.25, 0.0, 1.0);
  surface = mix(surface, u_FoamColor, foamColorMix);
  surface = ACESFilm(max(surface, vec3(0.0)));
  surface = pow(max(surface, vec3(0.0)), vec3(1.0 / 2.2));

  float alpha = hit.side ? u_Opacity * mix(0.86, 0.98, sideVolume)
                         : u_Opacity * mix(0.86, 0.995, depthFactor);
  if (!hit.side) {
    float shoreCoverageFade =
        mix(1.0, shoreCoverage, clamp(u_ShoreSoftness, 0.0, 1.0));
    alpha *= shoreCoverageFade;
  }
  alpha = clamp(alpha, 0.0, 1.0);
  alpha = max(alpha, max(foam * 0.24, crashFoam * 0.92));
  FragColor = vec4(surface, alpha);
}
