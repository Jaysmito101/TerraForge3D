#include "Generators/GenerationManager.h"
#include "Data/ApplicationState.h"
#include "Base/ComputeShader.h"
#include "Utils/Utils.h"
#include "Profiler.h"

static float a, b, c, d, e;
static bool ch = false;

GenerationManager::GenerationManager(ApplicationState* appState)
{
	m_AppState = appState;
	a = 0.0f;
	b = 0.0f;
	c = 1.0f;
	bool success = false;
	m_GenerationShader = new ComputeShader(ReadShaderSourceFile(m_AppState->constants.shadersDir + PATH_SEPARATOR "generation" PATH_SEPARATOR "dummy.glsl", &success));
}

GenerationManager::~GenerationManager()
{
	delete m_GenerationShader;
}

void GenerationManager::Update()
{
	if (ch)
	{
		START_PROFILER();
		m_GenerationShader->Bind();
		m_GenerationShader->SetUniform1f("a", a);
		m_GenerationShader->SetUniform1f("b", b);
		m_GenerationShader->SetUniform1f("c", c);
		m_GenerationShader->SetUniform1i("r", m_AppState->mainMap.tileResolution);
		m_GenerationShader->Dispatch(m_AppState->mainMap.tileResolution / 16, m_AppState->mainMap.tileResolution / 16, 1);
		END_PROFILER(d);
		e = d;
		ch = false;
	}
}

void GenerationManager::ShowSettings()
{
	ImGui::Begin("Generation Manager", &m_IsWindowVisible);
	ImGui::Text("Generation Manager");
	ch = ImGui::SliderFloat("a", &a, 0.0f, 1.0f) || ch;
	ch = ImGui::SliderFloat("b", &b, 0.0f, 1.0f) || ch; 
	ch = ImGui::SliderFloat("c", &c, 0.0f, 1.0f) || ch;
	ImGui::Text("Time: %f", e);
	ImGui::End();
}

void GenerationManager::InvalidateWorkInProgress()
{
}

bool GenerationManager::IsWorking()
{
	return false;
}
