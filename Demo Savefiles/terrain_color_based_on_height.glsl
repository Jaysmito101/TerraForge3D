#version 330 core
out vec4 FragColor;

uniform vec3 _LightPosition;
uniform vec3 _LightColor;

uniform vec3 _MousePos;

uniform float _Time;


in vec3 FragPos;
in vec3 Normal;

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

// Colors
vec3 red = vec3(1.0f, 0.0f, 0.0f);
vec3 green = vec3(0.0f, 1.0f, 0.0f);
vec3 blue = vec3(0.0f, 0.0f, 1.0f);
vec3 white = vec3(1.0f, 1.0f, 1.0f);
vec3 black = vec3(0.0f, 0.0f, 0.0f);
vec3 dgreen = vec3(0.062f, 0.396f, 0.133f);
vec3 seablue = vec3(0.601f, 0.774f, 1.0f);
vec3 brown = vec3(0.407f, 0.192f, 0.152f);

float cml(float val, float min, float max){
	if(val<min)
		return min;
	if(val>max)
		return max;
	return val;
}

// I am not very sure how to do this correctly. This is just what I was playing with.
// One big problem is I cannot figure out how to blend the boundaries properly.
vec3 getHeightBasedColor(vec3 pos){
	vec3 outCol = vec3(0.0f);
	float ns =noise(pos*100);
	float y = (pos.y*5)+ns*0.5f;
	if(y < 0.05f)
		outCol = outCol + blue ;
	else if(y < 0.50f)
		outCol = outCol + seablue;
	else if(y < 3.5f)
		outCol = outCol+  dgreen;
	else if(y < 5.0f)
		outCol = outCol +   green;
	else if(y > 5.0f)
		outCol = outCol +  brown;
	return outCol;
}


void main()
{	
	vec3 objectColor = vec3(1, 1, 1);
	objectColor = getHeightBasedColor(FragPos);
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(_LightPosition - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * _LightColor;
	vec3 result = (vec3(0.2, 0.2, 0.2) + diffuse) * objectColor;
	vec3 tmp = (vec3(sin(_Time))-FragPos);
	FragColor = vec4(result, 1.0);
	FragColor = vec4(tmp*result, 1.0);
} 

