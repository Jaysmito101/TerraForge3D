#pragma once

#include <string>
#include <vector>

#include "json.hpp"

#include "Base/OpenCL/ComputeKernel.h"

#define MAX_GENERATOR_MASKS 128

#pragma pack(push, 1)
struct GeneratorMask
{
	float type;

	float pos[3];

	float d1[4];
	float d2[4];
	float d3[4];
	float d4[4];
};
#pragma pack(pop)

enum GeneratorMaskType
{
	GeneratorMask_Additive = 0,
	GeneratorMask_AverageAdditive,
	GeneratorMask_Multiplicative,
	GeneratorMask_AverageMultiplicative,
	GeneratorMask_Count
};

static const char *generator_mask_type_names[] = {"Additive", "Average Additive", "Multiplicative", "Average Multiplicative"};

#define MASK_LAYER_HILL 0.0f
#define MASK_LAYER_CRATOR 1.0f
#define MASK_LAYER_CLIFF 2.0f

struct ApplicationState;

class GeneratorMaskManager
{
public:
	GeneratorMaskManager(ComputeKernel *kernel, std::string uid, ApplicationState *appState);
	~GeneratorMaskManager();

	nlohmann::json SaveGeneratorMask(GeneratorMask mask);
	GeneratorMask LoadGeneratorMask(nlohmann::json data);

	nlohmann::json Save();
	void Load(nlohmann::json data);

	float EvaluateAt(float x, float y, float z, float value);

	void ShowSettings();

//	void SetupForOpenCLEvaluation(ComputeKernel* kernel);


	bool enabled = false;
	std::vector<GeneratorMask> masks;
	std::string uid;
	int gmcount = 0;
	GeneratorMaskType type = GeneratorMaskType::GeneratorMask_Additive;
	ApplicationState *appState;
};