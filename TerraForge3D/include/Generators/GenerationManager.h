#pragma once

class ApplicationState;
class ComputeShader;

class GenerationManager
{
public:
	GenerationManager(ApplicationState* appState);
	~GenerationManager();

	void Update();
	void ShowSettings();
	void InvalidateWorkInProgress();
	bool IsWorking();

	inline const bool IsWindowVisible() const { return m_IsWindowVisible; }
	inline void SetWindowVisible(bool visible) { m_IsWindowVisible = visible; }
	inline bool* IsWindowVisiblePtr() { return &m_IsWindowVisible; }


private:
	ApplicationState* m_AppState = nullptr;
	ComputeShader* m_GenerationShader = nullptr;
	bool m_IsWindowVisible = true;
};