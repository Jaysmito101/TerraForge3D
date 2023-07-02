#pragma once

#include "Renderer/RendererViewport.h"

class ApplicationState;

class ViewportManager
{
public:
	ViewportManager(ApplicationState* appState);
	~ViewportManager();

	void Update();
	void Show();

	inline bool IsVisible() { return this->m_IsVisible; }
	inline void SetVisible(bool visible) { this->m_IsVisible = visible; }
	inline uint32_t GetID() { return this->m_ID; }

	inline RendererViewport* GetRendererViewport() { return this->m_RendererViewport; }
	inline bool IsActive() { return this->m_IsActive; }
	inline const glm::vec2 GetPositionOnTerrain() { return m_IsActive ? glm::vec2(m_RendererViewport->m_PosOnTerrain[0], m_RendererViewport->m_PosOnTerrain[1]) : glm::vec2(-1.0f); }

private:
	void ShowSettingPopUp();
	void ShowTextureSlotDetailsPopup();

private:
	ApplicationState* m_AppState = nullptr;
	RendererViewport* m_RendererViewport = nullptr;
	uint32_t m_ID = 0;
	float m_Width = 512.0f, m_Height = 512.0f;
	float m_MousePosX = 0.0f, m_MousePosY = 0.0f;
	float m_ZoomSpeed = 1.0f, m_MovementSpeed = 1.0f, m_RotationSpeed = 1.0f;
	bool m_IsVisible = true;
	bool m_AutoCalculateAspectRatio = true;
	bool m_IsActive = false;
};