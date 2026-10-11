#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Renderer/BrushSettings.h"

#include <optional>
#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)
TF3D_FWD_DEC_CLASS(GeneratorTexture, tf3d::generators)

namespace tf3d::generators
{

    enum class MaskStrokeMode : int {
        Paint         = 0,
        Erase         = 1,
        PaintNegative = 2
    };

    struct MaskStroke {
        std::vector<glm::vec2> points;
        float strength = 0.0f;
        float size     = 0.0f;
        float falloff  = 0.0f;
        MaskStrokeMode mode = MaskStrokeMode::Paint;
    };

    class MaskTool
    {
    public:
        struct State {
            std::vector<MaskStroke> strokes;
            std::optional<MaskStroke> activeStroke;
        };

        MaskTool(tf3d::data::ApplicationState *state, glm::vec3 vizColor,
                 bool allowNegativeValues = false);
        ~MaskTool();

        MaskTool(const MaskTool &)            = delete;
        MaskTool &operator=(const MaskTool &) = delete;
        MaskTool(MaskTool &&other) noexcept;
        MaskTool &operator=(MaskTool &&other) noexcept;

        void OnResolutionChanged();

        bool ShowSettings(tf3d::generators::GeneratorTexture *maskTexture,
                          tf3d::generators::GeneratorTexture *previewTexture,
                          bool showViewportMask = true);
        bool HideViewportOverlay(tf3d::generators::GeneratorTexture *maskTexture);
        State GetState() const;
        bool BeginStroke(MaskStroke stroke);
        bool AppendStrokePoints(const std::vector<glm::vec2> &points);
        bool FinishStroke();
        inline bool UndoStroke()
        {
            return UndoLastStroke();
        }
        bool ClearStrokes();
        SerializerNode Save() const;
        void Load(SerializerNode data);

        inline void SetVizColor(float r, float g, float b)
        {
            m_VizColor = glm::vec3(r, g, b);
        }
        inline void SetInvertPreview(bool invert)
        {
            m_InvertPreview = invert;
        }
        inline bool GetInvertPreview() const
        {
            return m_InvertPreview;
        }
        inline bool GetPreviewVisible() const
        {
            return m_PreviewVisible;
        }
        inline void SetPreviewSettings(bool visible,
                                       bool inverted,
                                       tf3d::generators::GeneratorTexture *maskTexture)
        {
            m_PreviewVisible = visible;
            m_InvertPreview  = inverted;
            UpdateViewportOverlay(maskTexture, false, m_PreviewVisible);
        }
        inline const std::vector<MaskStroke> &GetStrokes() const
        {
            return m_Strokes;
        }
        inline const MaskStroke *GetActiveStroke() const
        {
            return m_HasActiveStroke ? &m_ActiveStroke : nullptr;
        }

    private:
        void UpdateViewportOverlay(tf3d::generators::GeneratorTexture *maskTexture,
                                   bool showBrush, bool showMask);
        bool ShowStrokeSettings(int resolution);
        bool UndoLastStroke();
        bool ApplyDrawing(int resolution);
        void FinishActiveStroke();
        void StartActiveStroke(const glm::vec2 &position);
        void AppendActiveStrokePoint(const glm::vec2 &position, int resolution);

        tf3d::data::ApplicationState *m_AppState = nullptr;
        bool m_AllowNegativeValues               = false;
        std::vector<MaskStroke> m_Strokes;
        MaskStroke m_ActiveStroke;
        bool m_HasActiveStroke = false;

        glm::vec3 m_VizColor = glm::vec3(0.2f, 0.2f, 0.2f);
        bool m_InvertPreview = false;
        bool m_PreviewVisible = true;
        renderer::DrawBrushSettings m_DrawSettings;

        bool m_RequireUpdation  = true;
        bool m_IsEditing        = false;
        int m_PreviousBrushMode = 0;
        uint64_t m_Id           = 0;

        static MaskTool *s_CurrentlyEditingMaskTool;
    };

} // namespace tf3d::generators
