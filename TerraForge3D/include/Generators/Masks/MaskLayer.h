#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Exporters/Serializer.h"
#include "Generators/Masks/BaseMaskGenerator.h"
#include "Generators/Masks/MaskRasterizer.h"
#include "Generators/Masks/MaskTool.h"

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
                  std::string defaultTypeID = "None");
        ~MaskLayer() = default;

        MaskLayer(const MaskLayer &)            = delete;
        MaskLayer &operator=(const MaskLayer &) = delete;
        MaskLayer(MaskLayer &&other) noexcept;
        MaskLayer &operator=(MaskLayer &&other) noexcept;

        void Resize(int size);
        bool ShowSettings(bool showViewportMask = true);
        bool Update(const Snapshot *state, GeneratorData *sourceData);

        inline Snapshot GetState() const
        {
            return m_State.Capture();
        }
        inline Snapshot::Revision GetStateRevision() const
        {
            return m_State.PublishedRevision();
        }
        inline bool RequireUpdation() const
        {
            return m_State.RequiresUpdate();
        }

        void SaveTo(SerializerNode node) const;
        void LoadFrom(SerializerNode node);

        void SetInvertPreview(bool invert);
        void SetVizColor(float r, float g, float b);

        inline GeneratorTexture *GetTexture() const
        {
            return m_Texture.get();
        }

    private:
        static constexpr int32_t kMaxVisualizationResolution = 512;

        bool ShowBaseSettings();
        State CaptureState() const;
        bool Render(const State &state, GeneratorData *sourceData);
        void EnsureVisualizationTexture();

        tf3d::data::ApplicationState *m_AppState = nullptr;
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
