#include "ErosionFilter.h"
#include <imgui.h>
#include <iostream>

#define MAX(x, y) x>y?x:y
#define MIN(x, y) x<y?x:y

void ErosionFilter::Render()
{
	ImGui::DragInt("Num Iterations##erosionFilter", &iterations, 1, 1);
	ImGui::DragInt("Num Particles##erosionFilter", &numParticles, 1, 1);
	ImGui::DragFloat("Radius##erosionFilter", &radius, 0.1f, 1);
	ImGui::DragFloat("Deposition Rate##erosionFilter", &depositionRate, 0.1f, 0);
	ImGui::DragFloat("Erosion Rate##erosionFilter", &erosionRate, 0.1f, 0);
	ImGui::DragFloat("Iteration Scale##erosionFilter", &iterationScale, 0.1f, 0);
	ImGui::DragFloat("Friction##erosionFilter", &friction, 0.1f, 0);
	ImGui::DragFloat("Speed##erosionFilter", &speed, 0.1f, 0);

}

nlohmann::json ErosionFilter::Save()
{
	return nlohmann::json();
}

void ErosionFilter::Load(nlohmann::json data)
{
}

void ErosionFilter::trace(int x, int y) {
	int ox = (((double)rand() / (RAND_MAX)) * 2 - 1) * radius;
	int oy = (((double)rand() / (RAND_MAX)) * 2 - 1) * radius;
	float sediment = 0;
	int xp = x;
	int yp = y;
	float vx = 0;
	float vy = 0;

	for (int i = 0; i < iterations; i++) {
		if (y  <= 10)
			y = 10;

		glm::vec3 normal = model->mesh->GetNormals(x+ox, y+oy);
		if (normal.y == 1)
			break;

		float deposit = sediment * depositionRate * normal.y;
		float erosion = erosionRate * (1 - normal.y) * MIN(1, i * iterationScale);

		float elevation = model->mesh->GetElevation(xp, yp);
		model->mesh->SetElevation(elevation + deposit - erosion, xp, yp);

		sediment += erosion - deposit;

		vx = friction * vx + normal.x * speed;
		vy = friction * vy + normal.z * speed;
		xp = x;
		yp = y;
		x += vx;
		y += vy;
	}
}

void ErosionFilter::Apply()
{
	srand(time(NULL));
	model->mesh->RecalculateNormals();
	int res = model->mesh->res;
	res = res - 20;
	for (int i = 0; i < numParticles; i++) {
		int x = (int)(((double)rand() / (RAND_MAX)) * res + 10);
		int y = (int)(((double)rand() / (RAND_MAX)) * res + 10);
		trace(x, y);
		std::cout << "Processed " << i + 1 << " iterations.\r";
	}
	std::cout << "\n";
	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}
