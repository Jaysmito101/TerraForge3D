#version 330 core

layout (location = 0) in vec3 aPos;

layout (location = 1) in vec3 aNorm;

uniform mat4 _PV;
uniform float _Time;

out vec3 FragPos;

out vec3 Normal;

float hash(vec3 p)  // replace this by something better
{
    p  = fract( p*0.3183099+.1 );
	p *= 17.0;
    return fract( p.x*p.y*p.z*(p.x+p.y+p.z) );
}

float noise( in vec3 x )
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
	
    return mix(mix(mix( hash(i+vec3(0,0,0)), 
                        hash(i+vec3(1,0,0)),f.x),
                   mix( hash(i+vec3(0,1,0)), 
                        hash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix( hash(i+vec3(0,0,1)), 
                        hash(i+vec3(1,0,1)),f.x),
                   mix( hash(i+vec3(0,1,1)), 
                        hash(i+vec3(1,1,1)),f.x),f.y),f.z);
}


void main()
{
	float scale = 100;
	vec3 pos = aPos + aNorm*noise(aPos*10);
	gl_Position = _PV * vec4(pos.x, pos.y, pos.z, 1.0f);
	FragPos = aPos;
	Normal = aNorm;
}