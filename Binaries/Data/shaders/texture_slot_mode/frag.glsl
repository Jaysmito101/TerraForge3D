#version 430 core

out vec4 FragColor;


in VertexData
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
} fragmentInput;

layout(std430, binding = 0) buffer DataBuffer0
{
    vec4 data0[];
};

layout(std430, binding = 1) buffer DataBuffer1
{
    vec4 data1[];
};

layout(std430, binding = 2) buffer DataBuffer2
{
    vec4 data2[];
};

layout(std430, binding = 3) buffer DataBuffer3
{
    vec4 data3[];
};

layout(std430, binding = 4) buffer DataBuffer4
{
    vec4 data4[];
};

layout(std430, binding = 5) buffer DataBuffer5
{
    vec4 data5[];
};



uniform int u_Resolution;
uniform int u_SubTileSize;
uniform float u_TileSize;
uniform bool u_TextureSlotDetailedMode;
uniform int u_TextureSlot;
uniform ivec2 u_TextureSlotDetailed[4];

int PixelCoordToDataOffset(int x, int y)
{
	int tileSize = u_SubTileSize;
	int tileCount = u_Resolution / tileSize;
	int tileX = x / tileSize, tileY = y / tileSize;
	int tileXOffset = x % tileSize, tileYOffset = y % tileSize;
	int tileOffset = (tileY * tileCount + tileX) * (tileSize * tileSize);
	return (tileOffset + (tileYOffset * tileSize + tileXOffset));
}

float mapInRange(float value, float minV, float maxV, float newMin, float newMax)
{
	return newMin + (value - minV) * (newMax - newMin) / (maxV - minV);
}


vec4 SampleTexture(int index, int textureSlot)
{
	if(textureSlot == 0) return data0[index];
	else if(textureSlot == 1) return data1[index];
	else if(textureSlot == 2) return data2[index];
	else if(textureSlot == 3) return data3[index];
	else if(textureSlot == 4) return data4[index];
	else if(textureSlot == 5) return data5[index];
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