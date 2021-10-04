
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 _PV;
uniform mat4 _Model;
uniform float _Time;


// Copied from shadertoy
float hash(vec3 p)
{
    p  = fract( p*0.3183099+.1 );
	p *= 17.0;
    return fract( p.x*p.y*p.z*(p.x+p.y+p.z) );
}

// Copied from shadertoy
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


out DATA
{

    vec3 FragPos;
    vec3 Normal;
	vec2 TexCoord;
	mat4 PV;
} data_out; 

void main()
{
    gl_Position =  vec4(aPos.x, aPos.y + sin(5*_Time - 0.5*aPos.x*noise(vec3(aPos.x, 0, 0)))+sin(5*_Time - 0.5*aPos.z*noise(vec3(aPos.z, 0, 0))), aPos.z, 1.0);
	data_out.FragPos = aPos;
	data_out.Normal = aNorm;
	data_out.TexCoord = aTexCoord;
	data_out.PV = _PV;
}

