#include "utils/data.cl"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


float randi(float seedf)
{
	
	ulong seed = (ulong)seedf + get_global_id(0);
	seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
	uint random = seed >> 16;
	return (float)random;
}

float rand(float seed)
{
	float r1 = randi(seed);
	float r2 = randi(r1 + seed);
	float r3 = randi(r2 - seed);
	float r4 = randi(r3 + seed);
	float r = (r1 + r2)/(r3 + r4);
	if(r > 0.9999f)
	{
		r = clamp(r / 3.0f, 0.0f, 1.0f);
	}
	return r;
}

void cascade(int i, __global Vert* verts, float2 dim, WindParticle part)
{
  int size = dim.x*dim.y;

  //Neighbor Position Offsets (8-Way)
  const int nx[8] = {-1,-1,-1,0,0,1,1,1};
  const int ny[8] = {-1,0,1,-1,1,-1,0,1};

  //Neighbor Indices (8-Way
  int n[8] = {i-dim.y-1, i-dim.y, i-dim.y+1, i-1, i+1,
              i+dim.y-1, i+dim.y, i+dim.y+1};

  int2 ipos;

  //Iterate over all Neighbors
  for(int m = 0; m < 8; m++){

    ipos.x = (int)part.pos.x;
    ipos.y = (int)part.pos.y;

    //Neighbor Out-Of-Bounds
    if(n[m] < 0 || n[m] >= size) continue;
    if(ipos.x+nx[m] >= dim.x || ipos.y+ny[m] >= dim.y) continue;
    if(ipos.x+nx[m] < 0 || ipos.y+ny[m] < 0) continue;

    //Pile Size Difference and Excess
    float diff = (verts[i].position.y+verts[i].extras1.y) - (verts[n[m]].position.y+verts[n[m]].extras1.y);
    float excess = fabs(diff) - part.roughness;

    //Stable Configuration
    if(excess <= 0) continue; 

    float transfer = 0.0f;

    //Pile is Larger
    if(diff > 0) 
      transfer = MIN(verts[i].extras1.y, excess/2.0);

    //Neighbor is Larger
    else
      transfer = -MIN(verts[n[m]].extras1.y, excess/2.0);

    //Perform Transfer
    verts[i].extras1.y -= part.dt*part.settling*transfer;
    verts[n[m]].extras1.y += part.dt*part.settling*transfer;

  }

}


__kernel void erode(__global Vert *verts, __global WindParticle* parts)
{
	int i;
	i = get_global_id(0);
	
	WindParticle part = *parts;
	
	float shift = rand(part.seed) * (part.dim.x + part.dim.y - 1);
	if(shift < part.dim.x - 0.1)
	{
		part.pos.x = shift;
		part.pos.y = 1.0f;
	}
	else
	{
		part.pos.x = 1.0f;
		part.pos.y = shift - part.dim.x;
	}

//	printf("S: %f, X: %f, Y: %f\n\n", shift, part.pos.x, part.pos.y);

	part.index = part.pos.x * part.dim.y + part.pos.y;
	

	float2 ipos;
	float scale = 100.0f;

	for(i=0;;i++)
	{
		ipos = part.pos;
		int ind = ((int)(ipos.x * part.dim.y) + (int)ipos.y);
		if(part.height < verts[ind].position.y + verts[ind].extras1.y)
		{
			part.height = (verts[ind].position.y + verts[ind].extras1.y);
		}
		float3 n = verts[ind].normal.xyz;

		// Movement Mechanics
		if(part.height >  (verts[ind].position.y + verts[ind].extras1.y))
		{ // Flying
			part.speed.y -= part.dt*0.01f; // Gravity
		}
		else
		{ // Sliding
			float3 speedDelta = part.dt * cross(part.speed.xyz, n);
			part.speed.x += speedDelta.x;
			part.speed.y += speedDelta.y;
			part.speed.z += speedDelta.z;
		}
	
		// Accelerate by prevailing wind
		part.speed += 0.1f * part.dt * (part.pspeed - part.speed);	
		
		// Update poosition
		part.pos += part.dt * part.speed.xz;
		part.height = part.dt * part.speed.y;
		
		int nind = ((int)(part.pos.x * part.dim.y) + (int)part.pos.y);
		
		if(part.pos.x <= 0.0f || part.pos.y <= 0.0f || part.pos.x >= part.dim.x || part.pos.y >= part.dim.y)
		{
			break;
		}
		
		if(part.height <  (verts[nind].position.y + verts[nind].extras1.y))
		{
			float force = sqrt(part.speed.x * part.speed.x + part.speed.y * part.speed.y + part.speed.z * part.speed.z);
			force = force * ((verts[nind].position.y + verts[nind].extras1.y) - part.height);
			
			if(verts[ind].extras1.y <= 0)
			{
				verts[ind].extras1.y = 0.0f;
				verts[ind].position.y -= part.dt * part.abrasion * force * part.sediment * 10;
				verts[ind].extras1.y +=  part.dt * part.abrasion * force * part.sediment;
			}
			else if(verts[ind].extras1.y > part.dt * part.suspension * force)
			{
				verts[ind].extras1.y -= part.dt * part.suspension * force;	
				part.sediment += part.dt * part.suspension * force;
				cascade(ind, verts, part.dim, part);
			}
			else
			{
				verts[ind].extras1.y = 0.0f;
			}
		}
		else
		{
			part.sediment -= part.dt * part.suspension * part.sediment;
			verts[nind].extras1.y += 0.5f * part.dt * part.suspension * part.sediment;
			verts[ind].extras1.y += 0.5f * part.dt * part.suspension * part.sediment;
			
			cascade(nind, verts, part.dim, part);
			cascade( ind, verts, part.dim, part);
		}
		
		// Particle has no speed
		if(sqrt(part.speed.x * part.speed.x + part.speed.y * part.speed.y + part.speed.z * part.speed.z) < 0.01f)
		{
			break;
		}
	}

}