#include "Filters/ErosionFilter.h"
#include <imgui.h>
#include <iostream>

#include "Data/ApplicationState.h"

#define MAX(x, y) x>y?x:y
#define MIN(x, y) x<y?x:y

struct HeightAndGradient
{
	HeightAndGradient(float x, float y, float z)
	{
		height = x;
		gradientX = y;
		gradientY = z;
	}

	float height;
	float gradientX;
	float gradientY;
};

void ErosionFilter::Render()
{
	ImGui::DragInt("Max Droplet Lifetime##erosionFilter", &maxDropletLifetime, 1, 1);
	ImGui::DragInt("Num Particles##erosionFilter", &numIterations, 1, 1);
	ImGui::DragInt("Erosoion Radius##erosionFilter", &erosionRadius, 1, 1);
	ImGui::DragFloat("Sediment Capacity Factor##erosionFilter", &sedimentCapacityFactor, 0.1f, 0);
	ImGui::DragFloat("Minimum Sediment Capacity##erosionFilter", &minSedimentCapacity, 0.1f, 0);
	ImGui::DragFloat("Inertia##erosionFilter", &inertia, 0.1f, 0);
	ImGui::DragFloat("Errode Speed##erosionFilter", &erodeSpeed, 0.1f, 0);
	ImGui::DragFloat("Evaporation Speed##erosionFilter", &evaporateSpeed, 0.1f, 0);
	ImGui::DragFloat("Deposition Speed##erosionFilter", &depositSpeed, 0.1f, 0);
	ImGui::DragFloat("Gravity##erosionFilter", &gravity, 0.1f, 0);
}

nlohmann::json ErosionFilter::Save()
{
	return nlohmann::json();
}

void ErosionFilter::Load(nlohmann::json data)
{
}

HeightAndGradient CalculateHeightAndGradient(float *nodes, int mapSize, float posX, float posY)
{
	int coordX = (int)posX;
	int coordY = (int)posY;
	// Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
	float x = posX - coordX;
	float y = posY - coordY;
	// Calculate heights of the four nodes of the droplet's cell
	int nodeIndexNW = coordY * mapSize + coordX;
	float heightNW = nodes[nodeIndexNW];
	float heightNE = nodes[nodeIndexNW + 1];
	float heightSW = nodes[nodeIndexNW + mapSize];
	float heightSE = nodes[nodeIndexNW + mapSize + 1];
	// Calculate droplet's direction of flow with bilinear interpolation of height difference along the edges
	float gradientX = (heightNE - heightNW) * (1 - y) + (heightSE - heightSW) * y;
	float gradientY = (heightSW - heightNW) * (1 - x) + (heightSE - heightNE) * x;
	// Calculate height with bilinear interpolation of the heights of the nodes of the cell
	float height = heightNW * (1 - x) * (1 - y) + heightNE * x * (1 - y) + heightSW * (1 - x) * y + heightSE * x * y;
	return HeightAndGradient( height, gradientX, gradientY);
}

void ErosionFilter::Apply()
{
	Model *model;

	if(appState->mode == ApplicationMode::TERRAIN)
	{
		model = appState->models.coreTerrain;
	}

	else if(appState->mode == ApplicationMode::CUSTOM_BASE)
	{
		model = appState->models.customBase;
	}

	else
	{
		return;
	}

	srand(time(NULL));
	model->mesh->RecalculateNormals();
	int mapSize = model->mesh->res;
	int radius = erosionRadius;
	// BEGIN InitializeBrushIndices(mapSize, erosionRadius);
	erosionBrushIndices = new int *[mapSize * mapSize];
	erosionBrushWeights = new float*[mapSize * mapSize];
	int *erosionBrushIndicesSizes = new int[mapSize * mapSize];
	int *xOffsets = new int[radius * radius * 4];
	int *yOffsets = new int[radius * radius * 4];
	float *weights = new float[radius * radius * 4];
	float weightSum = 0;
	int addIndex = 0;

	for (int i = 0; i < mapSize*mapSize; i++)
	{
		int centreX = i % mapSize;
		int centreY = i / mapSize;

		if (centreY <= radius || centreY >= mapSize - radius || centreX <= radius + 1 || centreX >= mapSize - radius)
		{
			weightSum = 0;
			addIndex = 0;

			for (int y = -radius; y <= radius; y++)
			{
				for (int x = -radius; x <= radius; x++)
				{
					float sqrDst = x * x + y * y;

					if (sqrDst < radius * radius)
					{
						int coordX = centreX + x;
						int coordY = centreY + y;

						if (coordX >= 0 && coordX < mapSize && coordY >= 0 && coordY < mapSize)
						{
							float weight = 1 - sqrt(sqrDst) / radius;
							weightSum += weight;
							weights[addIndex] = weight;
							xOffsets[addIndex] = x;
							yOffsets[addIndex] = y;
							addIndex++;
						}
					}
				}
			}
		}

		int numEntries = addIndex;
		erosionBrushIndices[i] = new int[numEntries];
		erosionBrushWeights[i] = new float[numEntries];
		erosionBrushIndicesSizes[i] = numEntries;

		for (int j = 0; j < numEntries; j++)
		{
			erosionBrushIndices[i][j] = (yOffsets[j] + centreY) * mapSize + xOffsets[j] + centreX;
			erosionBrushWeights[i][j] = weights[j] / weightSum;
		}
	}

	// END
	float *map = new float[model->mesh->vertexCount];
	float minHeightT = 100000000000000;
	float maxHeightT = -100000000000000;
	for (int i = 0; i < model->mesh->vertexCount; i++)
	{
		if(model->mesh->vert[i].position.y > maxHeightT)
			maxHeightT = model->mesh->vert[i].position.y;
		if(model->mesh->vert[i].position.y < minHeightT)
			minHeightT = model->mesh->vert[i].position.y;
	}
	for (int i = 0; i < model->mesh->vertexCount; i++)
	{
		map[i] = (model->mesh->vert[i].position.y - minHeightT) / (maxHeightT - minHeightT);
	}

	currentErosionRadius = erosionRadius;
	currentMapSize = mapSize;
	std::cout << std::endl << "Starting processing erosion." << std::endl;

	for (int iteration = 0; iteration < numIterations; iteration++)
	{
		// Create water droplet at random point on map
		float posX = (int)(((double)rand() / (RAND_MAX)) * mapSize-1);
		float posY = (int)(((double)rand() / (RAND_MAX)) * mapSize - 1);

		if (posX < 2)
		{
			posX = 2;
		}

		if (posY < 2)
		{
			posY = 2;
		}

		float dirX = 0;
		float dirY = 0;
		float speed = initialSpeed;
		float water = initialWaterVolume;
		float sediment = 0;

		for (int lifetime = 0; lifetime < maxDropletLifetime; lifetime++)
		{
			int nodeX = (int)posX;
			int nodeY = (int)posY;
			int dropletIndex = nodeY * mapSize + nodeX;
			// Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
			float cellOffsetX = posX - nodeX;
			float cellOffsetY = posY - nodeY;
			// Calculate droplet's height and direction of flow with bilinear interpolation of surrounding heights
			HeightAndGradient heightAndGradient = CalculateHeightAndGradient(map, mapSize, posX, posY);
			// Update the droplet's direction and position (move position 1 unit regardless of speed)
			dirX = (dirX * inertia - heightAndGradient.gradientX * (1 - inertia));
			dirY = (dirY * inertia - heightAndGradient.gradientY * (1 - inertia));
			// Normalize direction
			float len = sqrt(dirX * dirX + dirY * dirY);

			if (len != 0)
			{
				dirX /= len;
				dirY /= len;
			}

			posX += dirX;
			posY += dirY;

			// Stop simulating droplet if it's not moving or has flowed over edge of map
			if ((dirX == 0 && dirY == 0) || posX < 0 || posX >= mapSize - 1 || posY < 0 || posY >= mapSize - 1)
			{
				break;
			}

			// Find the droplet's new height and calculate the deltaHeight
			float newHeight = CalculateHeightAndGradient(map, mapSize, posX, posY).height;
			float deltaHeight = newHeight - heightAndGradient.height;
			// Calculate the droplet's sediment capacity (higher when moving fast down a slope and contains lots of water)
			float sedimentCapacity = MAX(-deltaHeight * speed * water * sedimentCapacityFactor, minSedimentCapacity);

			// If carrying more sediment than capacity, or if flowing uphill:
			if (sediment > sedimentCapacity || deltaHeight > 0)
			{
				// If moving uphill (deltaHeight > 0) try fill up to the current height, otherwise deposit a fraction of the excess sediment
				float amountToDeposit = (deltaHeight > 0) ? MIN(deltaHeight, sediment) : (sediment - sedimentCapacity) * depositSpeed;
				sediment -= amountToDeposit;
				// Add the sediment to the four nodes of the current cell using bilinear interpolation
				// Deposition is not distributed over a radius (like erosion) so that it can fill small pits
				map[dropletIndex] += amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY);
				map[dropletIndex + 1] += amountToDeposit * cellOffsetX * (1 - cellOffsetY);
				map[dropletIndex + mapSize] += amountToDeposit * (1 - cellOffsetX) * cellOffsetY;
				map[dropletIndex + mapSize + 1] += amountToDeposit * cellOffsetX * cellOffsetY;
			}

			else
			{
				// Erode a fraction of the droplet's current carry capacity.
				// Clamp the erosion to the change in height so that it doesn't dig a hole in the terrain behind the droplet
				float amountToErode = MIN((sedimentCapacity - sediment) * erodeSpeed, -deltaHeight);

				// Use erosion brush to erode from all nodes inside the droplet's erosion radius
				for (int brushPointIndex = 0; brushPointIndex < erosionBrushIndicesSizes[dropletIndex]; brushPointIndex++)
				{
					int nodeIndex = erosionBrushIndices[dropletIndex][brushPointIndex];
					float weighedErodeAmount = amountToErode * erosionBrushWeights[dropletIndex][brushPointIndex];
					float deltaSediment = (map[nodeIndex] < weighedErodeAmount) ? map[nodeIndex] : weighedErodeAmount;
					map[nodeIndex] -= deltaSediment;
					sediment += deltaSediment;
				}
			}

			// Update droplet's speed and water content
			speed = sqrt(speed * speed + deltaHeight * gravity);
			water *= (1 - evaporateSpeed);
		}

		if(iteration % 100000 == 0)
		{
			std::cout << "Processed " + std::to_string(iteration) << " particles.\r";
		}
	}

	std::cout << "Finished Terrain Erosion Simulation\n";

	for (int i = 0; i < model->mesh->vertexCount; i++)
	{
		float newHeight = (map[i] * (maxHeightT - minHeightT)) + minHeightT;
		model->mesh->vert[i].extras1.z = model->mesh->vert[i].position.y - newHeight;
		model->mesh->vert[i].position.y = newHeight;
	}

	// Deletion

	for (int i = 0; i < mapSize * mapSize; i++)
	{
		if (erosionBrushIndices[i])
		{
			delete[] erosionBrushIndices[i];
		}

		if (erosionBrushWeights[i])
		{
			delete[] erosionBrushWeights[i];
		}
	}

	delete[] erosionBrushIndices;
	delete[] erosionBrushWeights;
	delete[] xOffsets;
	delete[] yOffsets;
	delete[] weights;
	delete[] map;
	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}
