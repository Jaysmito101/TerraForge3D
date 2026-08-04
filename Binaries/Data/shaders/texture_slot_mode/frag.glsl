#version 430 core

out vec4 FragColor;


in VertexData
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
} fragmentInput;

layout(TF3D_FIELD_FORMAT, binding = 0) readonly uniform image2D data0;
layout(TF3D_FIELD_FORMAT, binding = 1) readonly uniform image2D data1;
layout(TF3D_FIELD_FORMAT, binding = 2) readonly uniform image2D data2;
layout(TF3D_FIELD_FORMAT, binding = 3) readonly uniform image2D data3;
layout(TF3D_FIELD_FORMAT, binding = 4) readonly uniform image2D data4;
layout(TF3D_FIELD_FORMAT, binding = 5) readonly uniform image2D data5;



uniform int u_Resolution;
uniform int u_SubTileSize;
uniform float u_TileSize;
uniform bool u_TextureSlotDetailedMode;
uniform int u_TextureSlot;
uniform ivec2 u_TextureSlotDetailed[4];

int PixelCoordToDataOffset(int x, int y)
{
	return y * u_Resolution + x;
}

float mapInRange(float value, float minV, float maxV, float newMin, float newMax)
{
	return newMin + (value - minV) * (newMax - newMin) / (maxV - minV);
}


vec4 SampleTexture(int index, int textureSlot)
{
	ivec2 coordinate = ivec2(index % u_Resolution, index / u_Resolution);
	if(textureSlot == 0) return vec4(imageLoad(data0, coordinate).r);
	else if(textureSlot == 1) return vec4(imageLoad(data1, coordinate).r);
	else if(textureSlot == 2) return vec4(imageLoad(data2, coordinate).r);
	else if(textureSlot == 3) return vec4(imageLoad(data3, coordinate).r);
	else if(textureSlot == 4) return vec4(imageLoad(data4, coordinate).r);
	else if(textureSlot == 5) return vec4(imageLoad(data5, coordinate).r);
	else return vec4(0.0f);
}

void main()
{
    int index = PixelCoordToDataOffset(int(fragmentInput.texCoord.x * u_Resolution), int(fragmentInput.texCoord.y * u_Resolution));
	FragColor = vec4(0.0f);
	if(u_TextureSlotDetailedMode)
	{
	   for(int i =0; i < 4 ; i++)
	   {
	       FragColor[i] = SampleTexture(index, u_TextureSlotDetailed[i].x)[u_TextureSlotDetailed[i].y];
	   }
	}
	else
	{
		FragColor = SampleTexture(index, u_TextureSlot);
	}
}
