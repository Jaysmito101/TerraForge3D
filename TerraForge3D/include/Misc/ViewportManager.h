#pragma once

#include "Renderer/RendererViewport.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::misc
{

    class ViewportManager
    {
    public:
        ViewportManager(ApplicationState *appState);
        ~ViewportManager();

        SerializerNode Save() const;
        void Load(const SerializerNode &data);

        void Update();
        void Show();

        inline bool IsVisible()
        {
            return this->m_IsVisible;
        }
        inline void SetVisible(bool visible)
        {
            this->m_IsVisible = visible;
        }
        inline uint32_t GetID()
        {
            return this->m_ID;
        }

        inline renderer::RendererViewport *GetRendererViewport()
        {
            return this->m_RendererViewport;
        }
        inline bool IsActive()
        {
            return this->m_IsActive;
        }
        inline const glm::vec2 GetPositionOnTerrain()
        {
            const auto &position = m_RendererViewport->GetPositionOnTerrain();
            return m_IsActive ? glm::vec2(position[0], position[1]) : glm::vec2(-1.0f);
        }
        inline bool IsControlEnabled()
        {
            return m_IsControlEnabled;
        }
        inline void SetControlEnabled(bool enabled)
        {
            m_IsControlEnabled = enabled;
        }
        inline float GetDisplayWidth() const
        {
            return m_Width;
        }
        inline float GetDisplayHeight() const
        {
            return m_Height;
        }
        inline bool IsAutoCalculateAspectRatio() const
        {
            return m_AutoCalculateAspectRatio;
        }
        inline void SetAutoCalculateAspectRatio(bool enabled)
        {
            m_AutoCalculateAspectRatio = enabled;
        }

    private:
        void ShowSettingPopUp();
        void ShowTextureSlotDetailsPopup();

    private:
        ApplicationState *m_AppState                   = nullptr;
        renderer::RendererViewport *m_RendererViewport = nullptr;
        uint32_t m_ID                                  = 0;
        float m_Width = 512.0f, m_Height = 512.0f;
        float m_MousePosX = 0.0f, m_MousePosY = 0.0f;
        float m_ZoomSpeed = 1.0f, m_MovementSpeed = 1.0f, m_RotationSpeed = 1.0f;
        bool m_IsVisible                = true;
        bool m_AutoCalculateAspectRatio = true;
        bool m_IsActive                 = false;
        bool m_IsControlEnabled         = true;
    };

} // namespace tf3d::misc
