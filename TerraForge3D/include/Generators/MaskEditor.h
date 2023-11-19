#pragma once

#include "Base/Base.h"
#include "Utils/Utils.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Exporters/Serializer.h"
#include "Misc/CustomInspector.h"

class ApplicationState;

class MaskEditor
{
public:

	MaskEditor(ApplicationState* state);
	~MaskEditor();

	void Resize(int size);

	bool ShowSettings();

private:
	ApplicationState* m_AppState = nullptr;

	std::shared_ptr<GeneratorTexture> m_GeneratorTexture;
	int m_Size = 256;
	bool m_RequireUpdation = true;
};