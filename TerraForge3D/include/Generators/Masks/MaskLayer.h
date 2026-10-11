#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Exporters/Serializer.h"
#include "Generators/Masks/BaseMaskGenerator.h"
#include "Generators/Masks/MaskRasterizer.h"
#include "Generators/Masks/MaskTool.h"
#include "Generators/GenerationContext.h"

#include <memory>
#include <string>
#include <utility>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class MaskLayer
    {
    public:
        struct State {
            BaseMaskGenerator::State base;
            MaskTool::State tool;

            State(BaseMaskGenerator::State baseState, MaskTool::State toolState)
                : base(std::move(baseState)),
                  tool(std::move(toolState))
            {
            }
        };
        using Snapshot = base::GeneratorState<State>::Snapshot;

        MaskLayer(tf3d::data::ApplicationState *appState, glm::vec3 vizColor,
                  std::string defaultTypeID = "None", bool allowNegativeValues = false);
        ~MaskLayer() = default;

        MaskLayer(const MaskLayer &)            = delete;
        MaskLayer &operator=(const MaskLayer &) = delete;
        MaskLayer(MaskLayer &&other) noexcept;
        MaskLayer &operator=(MaskLayer &&other) noexcept;

        void Resize(int size);
        bool ShowSettings(bool showViewportMask = true);
        bool HideViewportOverlay();
        bool Apply(const State &state,
                   const GenerationContext *context,
                   GeneratorData *sourceData);
        bool Update(const Snapshot *state,
                    const GenerationContext *context,
                    GeneratorData *sourceData);

        inline Snapshot GetState() const
        {
            return m_State.Capture();
        }
        inline Snapshot::Revision GetStateRevision() const
        {
            return m_State.PublishedRevision();
        }
        inline Snapshot::Revision GetProcessedRevision() const
        {
            return m_State.ProcessedRevision();
        }
        inline bool RequireUpdation() const
        {
            return m_State.RequiresUpdate();
        }

        inline int GetResolution() const
        {
            return m_Texture != nullptr ? m_Texture->GetWidth() : 0;
        }
        bool UpdateBaseMaskSettings(const SerializerNode &patch, std::string &error);
        bool BeginStroke(MaskStroke stroke);
        bool AppendStrokePoints(const std::vector<glm::vec2> &points);
        bool FinishStroke();
        bool UndoStroke();
        bool ClearStrokes();
        inline void SetPreviewSettings(bool visible, bool inverted)
        {
            m_MaskTool.SetPreviewSettings(visible, inverted, m_Texture.get());
        }
        inline bool GetPreviewVisible() const
        {
            return m_MaskTool.GetPreviewVisible();
        }
        inline bool GetInvertPreview() const
        {
            return m_MaskTool.GetInvertPreview();
        }

        void SaveTo(SerializerNode node) const;
        inline SerializerNode SaveBaseMaskSettings() const
        {
            return m_BaseMaskGenerator.Save();
        }
        void LoadFrom(SerializerNode node);
        bool ValidateBaseMask(const SerializerNode &settings) const;
        nlohmann::json GetBaseMaskSchema() const;

        void SetInvertPreview(bool invert);
        void SetVizColor(float r, float g, float b);

        inline bool AllowsNegativeValues() const
        {
            return m_AllowNegativeValues;
        }

        inline GeneratorTexture *GetTexture() const
        {
            return m_Texture.get();
        }

        inline std::shared_ptr<GeneratorTexture> GetTextureHandle() const
        {
            return m_Texture;
        }

    private:
        static constexpr int32_t kMaxVisualizationResolution = 512;

        bool ShowBaseSettings();
        State CaptureState() const;
        void PublishState();
        bool Render(const State &state,
                    const GenerationContext *context,
                    GeneratorData *sourceData);
        void EnsureVisualizationTexture();

        tf3d::data::ApplicationState *m_AppState = nullptr;
        bool m_AllowNegativeValues               = false;
        BaseMaskGenerator m_BaseMaskGenerator;
        MaskTool m_MaskTool;
        MaskRasterizer m_Rasterizer;
        State m_UIState;
        base::GeneratorState<State> m_State;
        std::shared_ptr<GeneratorTexture> m_Texture;
        std::unique_ptr<GeneratorTexture> m_BaseTexture;
        std::unique_ptr<GeneratorTexture> m_VisualizationTexture;
    };

} // namespace tf3d::generators
