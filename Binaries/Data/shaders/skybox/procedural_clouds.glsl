#version 330 core
  in vec3 TexCoords;
vec3 fsun = vec3(0, 0.2, 0.1);
out vec4 FragColor;
uniform float time = 0.0;
uniform float cirrus = 0.4;
uniform float cumulus = 0.8;
uniform float upf = 0.35;

const float Br = 0.0025;
const float Bm = 0.0003;
const float g = 0.9800;
const vec3 nitrogen = vec3(0.650, 0.570, 0.475);
const vec3 Kr = Br / pow(nitrogen, vec3(4.0));
const vec3 Km = Bm / pow(nitrogen, vec3(0.84));

float hash(float n) {
  return fract(sin(n) * 43758.5453123);
}

float noise(vec3 x) {
  vec3 f = fract(x);
  float n = dot(floor(x), vec3(1.0, 157.0, 113.0));
  return mix(mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
      mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
    mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
      mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}

const mat3 m = mat3(0.0, 1.60, 1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);
float fbm(vec3 p) {
  float f = 0.0;
  f += noise(p) / 2;
  p = m * p * 1.1;
  f += noise(p) / 4;
  p = m * p * 1.2;
  f += noise(p) / 6;
  p = m * p * 1.3;
  f += noise(p) / 12;
  p = m * p * 1.4;
  f += noise(p) / 24;
  return f;
}

void main() {
  vec3 pos = vec3(TexCoords.x, TexCoords.y+upf, TexCoords.z);

  // Atmosphere Scattering
  float mu = dot(normalize(pos), normalize(fsun));
  float rayleigh = 3.0 / (8.0 * 3.14) * (1.0 + mu * mu);
  vec3 mie = (Kr + Km * (1.0 - g * g) / (2.0 + g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5)) / (Br + Bm);

  vec3 day_extinction = exp(-exp(-((pos.y + fsun.y * 4.0) * (exp(-pos.y * 16.0) + 0.1) / 80.0) / Br) * (exp(-pos.y * 16.0) + 0.1) * Kr / Br) * exp(-pos.y * exp(-pos.y * 8.0) * 4.0) * exp(-pos.y * 2.0) * 4.0;
  vec3 night_extinction = vec3(1.0 - exp(fsun.y)) * 0.2;
  vec3 extinction = mix(day_extinction, night_extinction, -fsun.y * 0.2 + 0.5);
  FragColor.rgb = rayleigh * mie * extinction;

  // Cirrus Clouds
  float density = smoothstep(1.0 - cirrus, 1.0, fbm(pos.xyz / pos.y * 2.0 + time * 0.05)) * 0.3;
  FragColor.rgb = mix(FragColor.rgb, extinction * 4.0, density * max(pos.y, 0.0));

  // Cumulus Clouds
  for (int i = 0; i < 3; i++) {
    float density = smoothstep(1.0 - cumulus, 1.0, fbm((0.7 + float(i) * 0.01) * pos.xyz / pos.y + time * 0.3));
    FragColor.rgb = mix(FragColor.rgb, extinction * density * 5.0, min(density, 1.0) * max(pos.y, 0.0));
  }

  // Dithering Noise
  FragColor.rgb += noise(pos * 1000) * 0.01;
}