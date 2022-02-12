#version 430 core

layout(local_size_x = LAYOUT_SIZE_X, local_size_y = LAYOUT_SIZE_Y, local_size_z = LAYOUT_SIZE_Z) in;

layout (std430, binding = 0)  buffer HeightDataBuffer
{
	float map[];
};


layout (std430, binding = 1)  buffer BrushIndexOffsetDataBuffer
{
	int brushIndices[];
};

layout (std430, binding = 2)  buffer BrushWeightsDataBuffer
{
	float brushWeights[];
};

layout (std430, binding = 3)  buffer RandomIndexDataBuffer
{
	int randomIndices[];
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

// Returns vec3(gradientX, gradientY, height)
vec3 CalculateHeightAndGradient (float posX, float posY) {
    int coordX =  int(posX);
    int coordY =  int(posY);

    // Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
    float x = posX - coordX;
    float y = posY - coordY;

    // Calculate heights of the four nodes of the droplet's cell
    int nodeIndexNW = coordY * mapSize + coordX;
    float heightNW = map[nodeIndexNW];
    float heightNE = map[nodeIndexNW + 1];
    float heightSW = map[nodeIndexNW + mapSize];
    float heightSE = map[nodeIndexNW + mapSize + 1];

    // Calculate droplet's direction of flow with bilinear interpolation of height difference along the edges
    float gradientX = (heightNE - heightNW) * (1 - y) + (heightSE - heightSW) * y;
    float gradientY = (heightSW - heightNW) * (1 - x) + (heightSE - heightNE) * x;

    // Calculate height with bilinear interpolation of the heights of the nodes of the cell
    float height = heightNW * (1 - x) * (1 - y) + heightNE * x * (1 - y) + heightSW * (1 - x) * y + heightSE * x * y;

    return vec3(gradientX,gradientY,height);
}

float modI(float a,float b) {
    float m=a-floor((a+0.5)/b)*b;
    return floor(m+0.5);
}

void main(void){
    int id = int(gl_LocalInvocationIndex);
	int index = randomIndices[id.x];
    float posX = modI(float(index) , float(mapSize));
    float posY = float(index) / mapSize;
    float dirX = 0;
    float dirY = 0;
    float speed = startSpeed;
    float water = startWater;
    float sediment = 0;

    for (int lifetime = 0; lifetime < maxLifetime; lifetime ++) {
        int nodeX = int(posX);
        int nodeY = int(posY);
        int dropletIndex = nodeY * mapSize + nodeX;
        // Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
        float cellOffsetX = posX - nodeX;
        float cellOffsetY = posY - nodeY;

        // Calculate droplet's height and direction of flow with bilinear interpolation of surrounding heights
        vec3 heightAndGradient = CalculateHeightAndGradient(posX, posY);

        // Update the droplet's direction and position (move position 1 unit regardless of speed)
        dirX = (dirX * inertia - heightAndGradient.x * (1 - inertia));
        dirY = (dirY * inertia - heightAndGradient.y * (1 - inertia));
        // Normalize direction
        float len = max(0.01,sqrt(dirX * dirX + dirY * dirY));
        dirX /= len;
        dirY /= len;
        posX += dirX;
        posY += dirY;

        // Stop simulating droplet if it's not moving or has flowed over edge of map
        if ((dirX == 0 && dirY == 0) || posX < borderSize || posX > mapSize - borderSize || posY < borderSize || posY > mapSize - borderSize) {
            break;
        }

        // Find the droplet's new height and calculate the deltaHeight
        float newHeight = CalculateHeightAndGradient (posX, posY).z;
        float deltaHeight = newHeight - heightAndGradient.z;

        // Calculate the droplet's sediment capacity (higher when moving fast down a slope and contains lots of water)
        float sedimentCapacity = max(-deltaHeight * speed * water * sedimentCapacityFactor, minSedimentCapacity);
        
        // If carrying more sediment than capacity, or if flowing uphill:
        if (sediment > sedimentCapacity || deltaHeight > 0) {
            // If moving uphill (deltaHeight > 0) try fill up to the current height, otherwise deposit a fraction of the excess sediment
            float amountToDeposit = (deltaHeight > 0) ? min(deltaHeight, sediment) : (sediment - sedimentCapacity) * depositSpeed;
            sediment -= amountToDeposit;

            // Add the sediment to the four nodes of the current cell using bilinear interpolation
            // Deposition is not distributed over a radius (like erosion) so that it can fill small pits
            map[dropletIndex] += amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY);
            map[dropletIndex + 1] += amountToDeposit * cellOffsetX * (1 - cellOffsetY);
            map[dropletIndex + mapSize] += amountToDeposit * (1 - cellOffsetX) * cellOffsetY;
            map[dropletIndex + mapSize + 1] += amountToDeposit * cellOffsetX * cellOffsetY;
        }
        else {
            // Erode a fraction of the droplet's current carry capacity.
            // Clamp the erosion to the change in height so that it doesn't dig a hole in the terrain behind the droplet
            float amountToErode = min ((sedimentCapacity - sediment) * erodeSpeed, -deltaHeight);
            
            for (int i = 0; i < brushLength; i ++) {
                int erodeIndex = dropletIndex + brushIndices[i];

                float weightedErodeAmount = amountToErode * brushWeights[i];
                float deltaSediment = (map[erodeIndex] < weightedErodeAmount) ? map[erodeIndex] : weightedErodeAmount;
                map[erodeIndex] -= deltaSediment;
                sediment += deltaSediment;
            }
        }

        // Update droplet's speed and water content
        speed = sqrt (max(0,speed * speed + deltaHeight * gravity));
        water *= (1 - evaporateSpeed);
    }

}