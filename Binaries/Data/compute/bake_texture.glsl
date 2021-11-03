
#version 430 core

layout(local_size_x = LAYOUT_SIZE_X, local_size_y = LAYOUT_SIZE_Y, local_size_z = LAYOUT_SIZE_Z) in;

layout (std430, binding = 1)  buffer gOutput1
{
	float hMap[];
};


layout (std430, binding = 0)  buffer gOutput2
{
	float oMap[];
};


uniform sampler2D  _DiffuseTextures[NUM_TEXTURE_LAYERS];
uniform vec3  _DiffuseTexturesHeights[NUM_TEXTURE_LAYERS];
uniform vec3  _DiffuseTexturesData[NUM_TEXTURE_LAYERS];

void main(void){

	uint X =  gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x; 
	uint Y =  gl_WorkGroupID.y * gl_WorkGroupSize.y + gl_LocalInvocationID.y; 
	
	oMap[X * RES + Y * 3 + 0] = hMap[X * RES + Y]*255;
	oMap[X * RES + Y * 3 + 1] = hMap[X * RES + Y]*255;
	oMap[X * RES + Y * 3 + 2] = hMap[X * RES + Y]*255;

}