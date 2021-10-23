#include <Model.h>
#include <glm/glm.hpp>
#include <chrono>

struct SnowballErosionProperties {
	int SEED = 10;
	std::chrono::milliseconds tickLength = std::chrono::milliseconds(1000);
	glm::vec2 dim = glm::vec2(256, 256);  //Size of the heightmap array
	bool updated = false;                 //Flag for remeshing

	double scale = 60.0;                  //"Physical" Height scaling of the map
	//double heightmap[256][256] = { 0.0 };

	//Erosion Steps
	bool active = false;
	int remaining = 200000;
	int erosionstep = 1000;

	int cycles = 5;

	//Particle Properties
	float dt = 1.2;
	float density = 1.0;  //This gives varying amounts of inertia and stuff...
	float evapRate = 0.001;
	float depositionRate = 0.1;
	float minVol = 0.01;
	float friction = 0.05;
};

SnowballErosionProperties* GetSnowballErosionProps();

void RenderSnowballErosionFilterPropUI();

void ApplySnowballErosion(Model* model);