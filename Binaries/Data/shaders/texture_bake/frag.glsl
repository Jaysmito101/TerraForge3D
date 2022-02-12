#version 430 core
out vec4 FragColor;

#define NUM_TEXTURE_LAYERS 8

uniform vec3 _LightPosition;
uniform vec3 _LightColor;

in float height;
in float Distance;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D  _DiffuseTextures[NUM_TEXTURE_LAYERS];
uniform vec3  _DiffuseTexturesHeights[NUM_TEXTURE_LAYERS];
uniform vec3  _DiffuseTexturesData[NUM_TEXTURE_LAYERS];
uniform float _SeaLevel;


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


float getTextureInfluence(int i, vec2 coord) {
float h = height + noise(coord*_DiffuseTexturesData[i].x)*_DiffuseTexturesData[i].y;
  float midVal = (_DiffuseTexturesHeights[i].x + _DiffuseTexturesHeights[i].y)/2;
  float p = 0;
  if(h < midVal)
    p = _DiffuseTexturesHeights[i].x - height;
  if(h >= midVal)
    p =height -  _DiffuseTexturesHeights[i].y;
  
  return pow(2.713, -_DiffuseTexturesData[i].z*p);
}

vec4 GetTextureColorBasedOnHeight(vec2 coord){
    vec4 accum = vec4(0.0);
    float total_influence = 0.0;
    
    for(int i=0; i < NUM_TEXTURE_LAYERS ; i++){
        float texture_influence = getTextureInfluence(i, coord*_DiffuseTexturesHeights[i].z);

        total_influence += texture_influence;
        accum += texture(_DiffuseTextures[i],  coord*_DiffuseTexturesHeights[i].z) * texture_influence;
    }

    if(total_influence > 0) {
      accum /= total_influence ;
    }
    return accum;
}


void main()
{	

	vec3 objectColor = vec3(1, 1, 1);
	objectColor = GetTextureColorBasedOnHeight(TexCoord).xyz;
    FragColor = vec4(objectColor, 1.0f);
} 