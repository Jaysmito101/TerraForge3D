#include <cmath>
#include <string>

#include "imgui/imgui.h"
#include "FastNoiseLite.h"

#include "GeneratorMask.h"

#define MAKE_IMGUI_ID(label, id) (label + std::string("##GMSKNL") + id).c_str()


// MATH UTILS

#define SQUARE(x) (x) * (x)

#define MIN(x, y) ((x) < (y)) ? (x) : (y)
#define MAX(x, y) ((x) > (y)) ? (x) : (y)

#define MATH_E 2.718281828459045
#define MATH_PI 3.1415926535897932384626433832795

#define M_PI_4 (MATH_PI/4.0)

#define Aatan 0.0776509570923569
#define Batan -0.287434475393028
#define Catan (M_PI_4 - Aatan - Batan)

inline float Fast2ArcTan(float x)
{
	float xx = x * x;
	return ((Aatan * xx + Batan) * xx + Catan) * x;
}

inline float smoothMin(float a, float b, float k)
{
	k = MAX(0, k);
	// https://www.iquilezles.org/www/articles/smin/smin.htm
	float h = MAX(0, MIN(1, (b - a + k) / (2 * k)));
	return a * h + b * (1 - h) - k * h * (1 - h);
}

// Smooth maximum of two values, controlled by smoothing factor k
// When k = 0, this behaves identically to max(a, b)
inline float smoothMax(float a, float b, float k)
{
	k = MIN(0, -k);
	float h = MAX(0, MIN(1, (b - a + k) / (2 * k)));
	return a * h + b * (1 - h) - k * h * (1 - h);
}


// MATH UTILS

static FastNoiseLite noiseGen;

inline bool ShowHillMaskSettingS(GeneratorMask *mask, std::string id)
{
	bool stateChanged = false;

	ImGui::Text("Mode : Hill");
	stateChanged = ImGui::DragFloat3(MAKE_IMGUI_ID("Position", id), mask->pos, 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Radius", id), &mask->d1[0], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Height", id), &mask->d1[1], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat2(MAKE_IMGUI_ID("Noise Offset", id), &mask->d2[0], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Noise Strength", id), &mask->d2[3], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Noise Frequency", id), &mask->d2[2], 0.01f) || stateChanged;
	ImGui::Text("Axis (0:XZ, 1:YX, 2:XY) : %f", mask->d1[3]);

	if(ImGui::Button( MAKE_IMGUI_ID("Change Axis", id) ) )
	{
		mask->d1[3] += 1.0f;
		if(mask->d1[3] == 3.0f)mask->d1[3] = 0.0f;
		stateChanged |= true;
	}
	return stateChanged;
}

inline float EvaluateHillMask(const GeneratorMask *mask, float x, float y, float z)
{
	float X;
	float Y;

	if(mask->d1[3] == 0.0f)
	{
		X = x - mask->pos[0];
		Y = z - mask->pos[2];
	}

	else if(mask->d1[3] == 1.0f)
	{
		X = y - mask->pos[1];
		Y = z - mask->pos[2];
	}

	else if(mask->d1[3] == 2.0f)
	{
		X = x - mask->pos[0];
		Y = y - mask->pos[1];
	}

	float theta = atan(Y/X);
	theta *= theta;
	X = X * X;
	Y = Y * Y;
	noiseGen.SetFrequency(mask->d2[2]);
	float invR = 1.0f / (mask->d1[0] + noiseGen.GetNoise(sin(theta) + mask->d2[0], cos(theta) + mask->d2[1]) * mask->d2[3]);
	float h = pow(MATH_E, -(X + Y) * invR);
	return h * (mask->d1[1] + mask->pos[1]);
}

inline bool ShowCratorMaskSettingS(GeneratorMask *mask, std::string id)
{
	bool stateChanged = false;
	ImGui::Text("Mode : Crater");
	stateChanged = ImGui::DragFloat3(MAKE_IMGUI_ID("Position", id), mask->pos, 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Radius", id), &mask->d1[0], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Floor", id), &mask->d3[3], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Rim Width", id), &mask->d1[3], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Rim Steepness", id), &mask->d3[0], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Smoothness", id), &mask->d3[1], 0.01f) || stateChanged;
	return stateChanged;
}

inline float EvaluateCratorMask(const GeneratorMask *mask, float x, float y, float z)
{
	float X = (x - mask->pos[0]);
	float Y = (z - mask->pos[2]);
	float l = sqrt(X*X + Y*Y) / mask->d1[0];
	float cavity = (l * l - 1);
	float rimX = MIN(l - 1 - mask->d1[3], 0.0f);
	float rim = mask->d3[0] * rimX * rimX;
	float cratershape = smoothMax(cavity, mask->d3[3], mask->d3[1]);
	cratershape = smoothMin(cratershape, rim,  mask->d3[1]);
	return cratershape * mask->d1[0] ;
	//return crater * mask->d3[2];
	return 0.0f;
}

inline bool ShowCliffMaskSettingS(GeneratorMask *mask, std::string id)
{
	bool stateChanged = false;
	ImGui::Text("Mode : Cliff");
	stateChanged = ImGui::DragFloat3(MAKE_IMGUI_ID("Position", id), mask->pos, 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Depth", id), &mask->d1[0], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Height", id), &mask->d1[1], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Steepness", id), &mask->d1[2], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Angle", id), &mask->d2[3], 0.01f) || stateChanged;
	ImGui::Text("Axis (0:XY, 1:YZ, 2:ZX) : %f", mask->d1[3]);

	if(ImGui::Button( MAKE_IMGUI_ID("Change Axis", id) ) )
	{
		mask->d1[3] += 1.0f;
		if(mask->d1[3] == 3.0f) mask->d1[3] = 0.0f;
		stateChanged = true;
	}
	return stateChanged;
}


inline float EvaluateCliffMask(const GeneratorMask *mask, float x, float y, float z)
{
	float X;
	float Y;

	if(mask->d1[3] == 0.0f)
	{
		X = x - mask->pos[0];
		Y = y - mask->pos[1];
	}

	else if(mask->d1[3] == 1.0f)
	{
		X = y - mask->pos[1];
		Y = z - mask->pos[2];
	}

	else if(mask->d1[3] == 2.0f)
	{
		X = z - mask->pos[2];
		Y = x - mask->pos[0];
	}

	float steepness = mask->d1[2];
	float height = mask->d1[1];
	float depth = mask->d1[0];
	float f = -steepness*X*X + height;
	float g = height;

	if(X >= 0)
	{
		g = depth;
	}

	return MAX(f, g);
}

inline bool ShowPlatueMaskSettingS(GeneratorMask *mask, std::string id)
{
	bool stateChanged = false;
	ImGui::Text("Mode : Platue");
	stateChanged = ImGui::DragFloat3(MAKE_IMGUI_ID("Position", id), mask->pos, 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Depth", id), &mask->d1[0], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Height", id), &mask->d1[1], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Steepness", id), &mask->d1[2], 0.01f) || stateChanged;
	stateChanged = ImGui::DragFloat(MAKE_IMGUI_ID("Angle", id), &mask->d2[3], 0.01f) || stateChanged;
	return stateChanged;
}

inline float EvaluatePlataueMask(const GeneratorMask *mask, float x, float y, float z)
{
	float X;
	float Y;

	if(mask->d1[3] == 0.0f)
	{
		X = x - mask->pos[0];
		Y = y - mask->pos[1];
	}

	else if(mask->d1[3] == 1.0f)
	{
		X = y - mask->pos[1];
		Y = z - mask->pos[2];
	}

	else if(mask->d1[3] == 2.0f)
	{
		X = z - mask->pos[2];
		Y = x - mask->pos[0];
	}

	float steepness = mask->d1[2];
	float height = mask->d1[1];
	float depth = mask->d1[0];
	float f = -steepness*X*X + height;
	float g = height;

	if(X >= 0)
	{
		g = depth;
	}

	return MAX(f, g);
}