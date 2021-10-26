
#version 430 core

layout(local_size_x = LAYOUT_SIZE_X, local_size_y = LAYOUT_SIZE_Y, local_size_z = LAYOUT_SIZE_Z) in;

layout (std430, binding = 0)  buffer gOutput
{
	float hMap[];
};

uniform int mapSize;
uniform int brushLength;
uniform int borderSize;

uniform int maxLifetime;
uniform float inertia;
uniform float sedimentCapacityFactor;
uniform float minSedimentCapacity;
uniform float depositSpeed;
uniform float erodeSpeed;

uniform float evaporateSpeed;
uniform float gravity;
uniform float startSpeed;
uniform float startWater;

void main(void){

	uint meshX =  gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x; 
	uint meshY =  gl_WorkGroupID.y * gl_WorkGroupSize.y + gl_LocalInvocationID.y; 
	
	hMap[meshX * MESH_RESOLUTION + meshY] = sin(meshX*0.01)*gravity;

}