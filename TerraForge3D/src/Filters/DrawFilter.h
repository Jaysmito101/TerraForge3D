#pragma once

#include "Filter.h"

class DrawFilter : public Filter
{

public:
	DrawFilter(Model *model)
		:Filter(model, "Draw Filter") {}

	virtual void Render() override;
	virtual nlohmann::json Save() override;
	virtual void Load(nlohmann::json data) override;
	virtual void Apply() override;

	float strength = 0.1f;
	float radius = 5.0f;
};
