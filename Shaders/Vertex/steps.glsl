#version 330 core

layout (location = 0) in vec3 aPos;

layout (location = 1) in vec3 aNorm;

uniform mat4 _PV;
uniform float _Time;

out vec3 FragPos;

out vec3 Normal;


vec3 hash3( vec2 p )
{
    vec3 q = vec3( dot(p,vec2(127.1,311.7)), 
				   dot(p,vec2(269.5,183.3)), 
				   dot(p,vec2(419.2,371.9)) );
	return fract(sin(q)*43758.5453);
}

float voronoise( in vec2 p, float u, float v )
{
	float k = 1.0+63.0*pow(1.0-v,6.0);

    vec2 i = floor(p);
    vec2 f = fract(p);
    
    vec2 a = vec2(0.0,0.0);
    for( int y=-2; y<=2; y++ )
    for( int x=-2; x<=2; x++ )
    {
        vec2  g = vec2( x, y );
		vec3  o = hash3( i + g )*vec3(u,u,1.0);
		vec2  d = g - f + o.xy;
		float w = pow( 1.0-smoothstep(0.0,1.414,length(d)), k );
		a += vec2(o.z*w,w);
    }
	
    return a.x/a.y;
}


void main()
{
	float scale = 100;
	float noise = voronoise(vec2(aPos.x, aPos.z)*10, cos(_Time), sin(_Time));
	gl_Position = _PV * vec4(aPos.x, aPos.y+noise, aPos.z, 1.0f);
	FragPos = aPos;
	Normal = aNorm;
}