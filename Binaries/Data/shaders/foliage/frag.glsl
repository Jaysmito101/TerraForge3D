#version 430 core
out vec4 FragColor;

#define NUM_TEXTURE_LAYERS 8

uniform vec3 _LightPosition;
uniform vec3 _LightColor;

in float height;
in float Distance;
in flat vec4 FragPos;
in vec3 Normal;
in vec2 TexCoord;

vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot( n, vec3(70.0) );
}


void main()
{	

	vec3 objectColor = vec3(0, 1, 1);
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos.xyz);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec3 result = (vec3(0.2, 0.2, 0.2) + diffuse) * objectColor;
	float factor = pow(2.7139, -1*Distance);
	FragColor = mix(vec4(result, 0.6f), vec4(1.0), 0.1);
	FragColor = vec4(result, 1.0f);
} 