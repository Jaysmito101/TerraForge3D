#pragma once

#include "Base/Base.h"

namespace tf3d::renderer
{
    struct DrawBrushSettings {
        float m_BrushSize      = 0.2f;
        float m_BrushStrength  = 0.01f;
        float m_BrushFalloff   = 0.5f;
        float m_BrushPositionX = 0.0f;
        float m_BrushPositionY = 0.0f;
        float m_BrushRotation  = 0.0f;
        int m_BrushMode        = 0;

        int32_t m_MaskTexture  = -1;
        glm::vec3 m_MaskColor  = glm::vec3(1.0f);
        bool m_ShowMask        = false;
        bool m_InvertMask      = false;
        bool m_SignedMask      = false;
        bool m_ShowBrushCursor = false;
    };
} // namespace tf3d::renderer
