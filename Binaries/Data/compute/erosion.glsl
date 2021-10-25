
#version 430 core

layout(local_size_x = LAYOUT_SIZE_X, local_size_y = LAYOUT_SIZE_Y, local_size_z = LAYOUT_SIZE_Z) in;

struct Vert 
{
	vec4 position;
	vec4 normal;
	vec2 texCoord;
};


layout (std430, binding = 0)  buffer gOutput
{
	float verts[];
};

void main(void){

	uint meshX =  gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x; 
	uint meshY =  gl_WorkGroupID.y * gl_WorkGroupSize.y + gl_LocalInvocationID.y; 
	
	//verts[meshX * MESH_RESOLUTION + meshY].position = vec4(0.0f);
	verts[meshX * MESH_RESOLUTION + meshY] = -3.0f;

}