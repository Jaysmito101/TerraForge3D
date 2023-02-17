#include "utils/data.cl"

#ifndef NOISE_CL
#define NOISE_CL


float fractf(float f){return f - floor(f);}

float3 fractf3(float3 f){return f - floor(f);}

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

float hash( float n )
{
    return fractf(sin(n)*43758.5453f);
}

float4 mod(float4 x, float y){ return x - y * floor(x/y);}
float3 mod3(float3 x, float y){ return x - y * floor(x/y);}

float4 permute(float4 x){return mod(((x*34.0f)+1.0f)*x, 289.0f);}
float4 taylorInvSqrt(float4 r){return 1.79284291400159f - 0.85373472095314f * r;}

float snoise(float3 v){ 
  float2  C;
  C.x = 1.0f/6.0f;
  C.y= 1.0f/3.0f;

  float4  D;
  D.x = 0.0f;
  D.y = 0.5f;
  D.z = 1.0f;
  D.w = 2.0f;

// First corner
  float3 i  = floor(v + dot(v, C.yyy) );
  float3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  float3 g = step(x0.yzx, x0.xyz);
  float3 l = 1.0f - g;
  float3 i1 = min( g.xyz, l.zxy );
  float3 i2 = max( g.xyz, l.zxy );

  //  x0 = x0 - 0. + 0.0f * C 
  float3 x1 = x0 - i1 + 1.0f * C.xxx;
  float3 x2 = x0 - i2 + 2.0f * C.xxx;
  float3 x3 = x0 - 1.0f + 3.0f * C.xxx;

float4 t1, t2, t3;
t1.x = t2.x = t3.x = 0.0f;
t1.w = t2.w = t3.w = 1.0f;

t1.y = i1.z;
t1.z = i2.z;

t2.y = i1.y;
t2.z = i2.y;

t3.y = i1.x;
t3.z = i2.x;

// Permutations
  i = mod3(i, 289.0f ); 
  float4 p = permute( permute( permute( 
             i.z + t1)
           + i.y + t2) 
           + i.x + t3);

// Gradients
// ( N*N points uniformly over a square, mapped onto an octahedron.)
  float n_ = 1.0f/7.0f; // N=7
  float3  ns = n_ * D.wyz - D.xzx;

  float4 j = p - 49.0f * floor(p * ns.z *ns.z);  //  mod(p,N*N)

  float4 x_ = floor(j * ns.z);
  float4 y_ = floor(j - 7.0f * x_ );    // mod(j,N)

  float4 x = x_ *ns.x + ns.yyyy;
  float4 y = y_ *ns.x + ns.yyyy;
  float4 h = 1.0f - fabs(x) - fabs(y);

  float4 b0;
  b0.x = x.x;
  b0.y = x.y;
  b0.z = y.x;
  b0.w = y.y;

  float4 b1;
  b1.x = x.z;
  b1.y = x.w;
  b1.z = y.z;
  b1.w = y.w;

  float4 s0 = floor(b0)*2.0f + 1.0f;
  float4 s1 = floor(b1)*2.0f + 1.0f;
  float4 sh = -step(h, 0.0f);

  float4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  float4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  float3 p0;
  p0.x = a0.x;
  p0.y = a0.y;
  p0.z = h.x;
  float3 p1;
  p1.x = a0.z;
  p1.y = a0.w;
  p1.z = h.y;
  float3 p2;
  p2.x = a1.x;
  p2.y = a1.y;
  p2.z = h.z;
  float3 p3;
  p3.x = a1.z;
  p3.y = a1.w;
  p3.z = h.w;
  float4 tmp;
  tmp.x = dot(p0,p0); 
  tmp.y = dot(p1,p1); 
  tmp.z = dot(p2,p2); 
  tmp.w = dot(p3,p3); 
//Normalise gradients
  float4 norm = taylorInvSqrt(tmp);
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;
  tmp.x = dot(x0,x0);
  tmp.y = dot(x1,x1);
  tmp.z = dot(x2,x2);
  tmp.w = dot(x3,x3);
// Mix final noise value
  float4 m = max(0.6f - tmp, 0.0f);
  m = m * m;
  tmp.x = dot(p0,x0);
  tmp.y = dot(p1,x1);
  tmp.z = dot(p2,x2);
  tmp.w = dot(p3,x3);
  return 42.0f * dot( m*m, tmp );
}

float noise(float3 x, float depth)
{
	float3 d = x;
	float n = 0.0f;
	for(float i=0;i<depth;i+=1.0f)	
	{
		n = snoise(x);	
		x += n;
	}
	return snoise(x);
}

float pingpong(float t)
{
	t -= (int)(t * 0.5f) * 2.0f;
	return t < 1 ? t : 2 - t;
}

float get_simple_noise(NoiseLayer nl){
	return noise(nl.value.xyz * nl.frequency + nl.offset.xyz, nl.depth) * nl.strength;
}

float get_fbm_noise(NoiseLayer nl){
	float amp =  1.0f / 1.75f;
	float sum = 0.0f;
	float3 val = nl.value.xyz * nl.frequency + nl.offset.xyz;

	for(int i=0;i<nl.octaves;i++)
	{
		float n = noise(val, nl.depth);
		sum += n * amp;
		amp *= lerp(1.0f, (n + 1.0f) * 0.5f, nl.weightedStrength);

		val *= nl.lacunarity;
		amp *= nl.gain;
	}

	return sum * nl.strength;
}

float get_ridged_noise(NoiseLayer nl){
	float amp =  1.0f / 1.75f;
	float sum = 0.0f;
	float3 val = nl.value.xyz * nl.frequency + nl.offset.xyz;

	for(int i=0;i<nl.octaves;i++)
	{
		float n = fabs(noise(val, nl.depth));
		sum += (n * -2.0f + 1.0f) * amp;
		amp *= lerp(1.0f, 1.0f - n, nl.weightedStrength);

		val *= nl.lacunarity;
		amp *= nl.gain;
	}

	return sum * nl.strength;	
}

float get_pingpong_noise(NoiseLayer nl){
	float amp =  1.0f / 1.75f;
	float sum = 0.0f;
	float3 val = nl.value.xyz * nl.frequency + nl.offset.xyz;

	for(int i=0;i<nl.octaves;i++)
	{
		float n = pingpong(noise(val, nl.depth) * nl.pingPongStrength);
		sum += (n - 0.5f) * 2.0f * amp;
		amp *= lerp(1.0f, n, nl.weightedStrength);

		val *= nl.lacunarity;
		amp *= nl.gain;
	}

	return sum * nl.strength;
}

float get_noise(NoiseLayer nl)
{
	if(nl.fractal == 0.0f)	
		return get_simple_noise(nl);
	else if (nl.fractal == 1.0f)	
		return get_fbm_noise(nl);
	else if (nl.fractal == 2.0f)	
		return get_ridged_noise(nl);
	else if (nl.fractal == 3.0f)	
		return get_pingpong_noise(nl);
	else
		return 0.0f;
}


#endif