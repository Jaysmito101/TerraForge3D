#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 cloudsBoxBoundsMin;
uniform vec3 cloudsBoxBoundsMax;

uniform sampler2D _ColorTexture;
uniform sampler2D _DepthTexture;

uniform vec3 _CameraPosition;
uniform vec3 _ViewDir;

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

void main()
{    
    vec4 col = texture(_ColorTexture, TexCoords);
    vec3 rayOrigin = _CameraPosition;
    vec3 rayDir = _ViewDir;

    float cld = 0;
    float tmp = 0;
    while(tmp < 1)
    {
    	tmp += 0.1f;
    	cld += noise((vec3(TexCoords, 0) + rayDir * tmp) * cloudsBoxBoundsMax.y);
    }

    tmp = exp(-cld) * 0.1;


    FragColor = mix(col, vec4(1), tmp * cloudsBoxBoundsMax.x);
}