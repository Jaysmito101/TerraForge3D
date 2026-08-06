#include "Generators/MaskTool.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Renderer/ObjectRenderer.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <cmath>

namespace tf3d::generators
{

    MaskTool *MaskTool::s_CurrentlyEditingMaskTool = nullptr;

    MaskTool::MaskTool(ApplicationState *state, glm::vec3 vizColor)
        : m_AppState(state), m_VizColor(vizColor)
    {
        m_BaseTexture                = std::make_shared<GeneratorTexture>(m_Size, m_Size, GeneratorTextureStorage::R16);
        m_PaintedTexture             = std::make_shared<GeneratorTexture>(m_Size, m_Size, GeneratorTextureStorage::R16);
        m_VisualizationTexture       = std::make_shared<GeneratorTexture>(m_Size, m_Size, GeneratorTextureStorage::R16);
        m_RasterizeShader            = m_AppState->resourceManager->LoadComputeShader("generation/utils/mask_editor");
        m_CopyShader                 = m_AppState->resourceManager->LoadComputeShader("generation/utils/mask_copy");
        m_StrokeSettingsBuffer       = std::make_shared<ShaderStorageBuffer>();
        m_StrokeRangesBuffer         = std::make_shared<ShaderStorageBuffer>();
        m_StrokePointsBuffer         = std::make_shared<ShaderStorageBuffer>();
        m_DrawSettings.m_MaskTexture = m_PaintedTexture->GetRendererID();
    }

    MaskTool::~MaskTool()
    {
        if (s_CurrentlyEditingMaskTool == this)
            s_CurrentlyEditingMaskTool = nullptr;
    }

    void MaskTool::Resize(int size)
    {
        if (size <= 0)
            return;
        m_Size = size;
        m_Strokes.clear();
        m_ActiveStroke    = MaskStroke{};
        m_HasActiveStroke = false;
        m_BaseTexture->Resize(size, size);
        m_PaintedTexture->Resize(size, size);
        m_VisualizationTexture->Resize(size, size);
    }

    void MaskTool::SetPreviewMode(MaskPreviewMode mode)
    {
        if (mode == MaskPreviewMode::Generated && m_ExternalGeneratedTexture == nullptr)
            return;
        if (m_PreviewMode == mode)
            return;
        FinishActiveStroke();
        m_PreviewMode     = mode;
        m_RequireUpdation = true;
    }

    void MaskTool::SetGeneratedMaskTexture(GeneratorTexture *texture, const char *label)
    {
        const bool sourceChanged   = m_ExternalGeneratedTexture != texture;
        m_ExternalGeneratedTexture = texture;
        if (label != nullptr)
            m_GeneratedMaskLabel = label;
        if (m_ExternalGeneratedTexture == nullptr && m_PreviewMode == MaskPreviewMode::Generated) {
            m_PreviewMode = MaskPreviewMode::Painted;
        }
        if (sourceChanged)
            m_RequireUpdation = true;
    }

    void MaskTool::ClearGeneratedMaskTexture()
    {
        m_ExternalGeneratedTexture = nullptr;
        m_GeneratedMaskLabel       = "Generated mask";
        if (m_PreviewMode == MaskPreviewMode::Generated)
            m_PreviewMode = MaskPreviewMode::Painted;
        m_RequireUpdation = true;
    }

    SerializerNode MaskTool::Save() const
    {
        auto node = CreateSerializerNode();
        node->Set("MaskSource", static_cast<int>(m_PreviewMode));
        return node;
    }

    void MaskTool::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;
        const int savedSource = data->Get<int>("MaskSource",
                                               data->Get<int>("PreviewMode", static_cast<int>(MaskPreviewMode::Painted)));
        const auto source     = static_cast<MaskPreviewMode>(glm::clamp(savedSource, 0, 1));
        FinishActiveStroke();
        m_PreviewMode = source == MaskPreviewMode::Generated && m_ExternalGeneratedTexture != nullptr
                            ? MaskPreviewMode::Generated
                            : MaskPreviewMode::Painted;
        m_IsEditing   = false;
        if (s_CurrentlyEditingMaskTool == this)
            s_CurrentlyEditingMaskTool = nullptr;
        m_RequireUpdation = true;
    }

    bool MaskTool::CopyGeneratedMaskToPainted()
    {
        if (m_ExternalGeneratedTexture == nullptr)
            return false;

        m_ExternalGeneratedTexture->Bind(0);
        m_BaseTexture->BindForCompute(1);
        m_CopyShader->Bind();
        m_CopyShader->SetUniform1i("u_Resolution", m_Size);
        m_CopyShader->SetUniform1i("u_SourceMask", 0);
        m_CopyShader->SetUniform1i("u_Invert", 0);
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_Size + workgroupSize - 1) / workgroupSize;
        m_CopyShader->Dispatch(dispatchSize, dispatchSize, 1);
        m_CopyShader->SetMemoryBarrier();
        glFinish();
        m_Strokes.clear();
        m_ActiveStroke    = MaskStroke{};
        m_HasActiveStroke = false;
        RasterizeStrokes();

        SetPreviewMode(MaskPreviewMode::Painted);
        if (s_CurrentlyEditingMaskTool != nullptr) {
            s_CurrentlyEditingMaskTool->FinishActiveStroke();
            s_CurrentlyEditingMaskTool->m_IsEditing = false;
        }
        m_IsEditing                = true;
        s_CurrentlyEditingMaskTool = this;
        m_RequireUpdation          = true;
        return true;
    }

    void MaskTool::UpdateVisualizationTexture(GeneratorTexture *sourceTexture)
    {
        if (sourceTexture == nullptr || !m_InvertPreview)
            return;

        sourceTexture->Bind(0);
        m_VisualizationTexture->BindForCompute(1);
        m_CopyShader->Bind();
        m_CopyShader->SetUniform1i("u_Resolution", m_Size);
        m_CopyShader->SetUniform1i("u_SourceMask", 0);
        m_CopyShader->SetUniform1i("u_Invert", 1);
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_Size + workgroupSize - 1) / workgroupSize;
        m_CopyShader->Dispatch(dispatchSize, dispatchSize, 1);
        m_CopyShader->SetMemoryBarrier();
    }

    void MaskTool::RasterizeStrokes()
    {
        std::vector<glm::vec4> settings;
        std::vector<glm::ivec4> ranges;
        std::vector<glm::vec4> points;

        auto appendStroke = [&](const MaskStroke &stroke) {
            if (stroke.points.empty())
                return;
            settings.emplace_back(stroke.strength, stroke.size, stroke.falloff, static_cast<float>(stroke.mode));
            ranges.emplace_back(static_cast<int>(points.size()), static_cast<int>(stroke.points.size()), 0, 0);
            for (const auto &point : stroke.points)
                points.emplace_back(point.x, point.y, 0.0f, 0.0f);
        };

        for (const auto &stroke : m_Strokes)
            appendStroke(stroke);
        if (m_HasActiveStroke)
            appendStroke(m_ActiveStroke);

        const glm::vec4 emptySettings(0.0f);
        const glm::ivec4 emptyRange(0);
        const glm::vec4 emptyPoint(0.0f);
        m_StrokeSettingsBuffer->SetData(settings.empty() ? (void *)&emptySettings : (void *)settings.data(),
                                        static_cast<unsigned int>((settings.empty() ? 1 : settings.size()) * sizeof(glm::vec4)));
        m_StrokeRangesBuffer->SetData(ranges.empty() ? (void *)&emptyRange : (void *)ranges.data(),
                                      static_cast<unsigned int>((ranges.empty() ? 1 : ranges.size()) * sizeof(glm::ivec4)));
        m_StrokePointsBuffer->SetData(points.empty() ? (void *)&emptyPoint : (void *)points.data(),
                                      static_cast<unsigned int>((points.empty() ? 1 : points.size()) * sizeof(glm::vec4)));

        m_BaseTexture->Bind(0);
        m_PaintedTexture->BindForCompute(0);
        m_StrokeSettingsBuffer->Bind(1);
        m_StrokeRangesBuffer->Bind(2);
        m_StrokePointsBuffer->Bind(3);
        m_RasterizeShader->Bind();
        m_RasterizeShader->SetUniform1i("u_Resolution", m_Size);
        m_RasterizeShader->SetUniform1i("u_BaseMask", 0);
        m_RasterizeShader->SetUniform1i("u_StrokeCount", static_cast<int>(settings.size()));
        const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
        const auto dispatchSize  = (m_Size + workgroupSize - 1) / workgroupSize;
        m_RasterizeShader->Dispatch(dispatchSize, dispatchSize, 1);
        m_RasterizeShader->SetMemoryBarrier();
        m_RequireUpdation = true;
    }

    void MaskTool::FinishActiveStroke()
    {
        if (!m_HasActiveStroke)
            return;
        if (!m_ActiveStroke.points.empty())
            m_Strokes.emplace_back(std::move(m_ActiveStroke));
        m_ActiveStroke    = MaskStroke{};
        m_HasActiveStroke = false;
        RasterizeStrokes();
    }

    bool MaskTool::UndoLastStroke()
    {
        if (m_HasActiveStroke) {
            m_ActiveStroke    = MaskStroke{};
            m_HasActiveStroke = false;
            RasterizeStrokes();
            return true;
        }
        if (m_Strokes.empty())
            return false;

        m_Strokes.pop_back();
        RasterizeStrokes();
        return true;
    }

    void MaskTool::StartActiveStroke(const glm::vec2 &position)
    {
        m_ActiveStroke          = MaskStroke{};
        m_ActiveStroke.strength = glm::clamp(m_DrawSettings.m_BrushStrength, 0.0f, 1.0f);
        m_ActiveStroke.size     = glm::max(m_DrawSettings.m_BrushSize, 0.000001f);
        m_ActiveStroke.falloff  = glm::clamp(m_DrawSettings.m_BrushFalloff, 0.0f, 1.0f);
        m_ActiveStroke.mode     = m_DrawSettings.m_BrushMode;
        m_ActiveStroke.points.push_back(position);
        m_HasActiveStroke = true;
    }

    void MaskTool::AppendActiveStrokePoint(const glm::vec2 &position)
    {
        if (!m_HasActiveStroke) {
            StartActiveStroke(position);
            return;
        }

        const float sampleSpacing = glm::max(0.5f / static_cast<float>(m_Size), 0.0005f);
        if (glm::length(position - m_ActiveStroke.points.back()) >= sampleSpacing)
            m_ActiveStroke.points.push_back(position);
    }

    bool MaskTool::ApplyDrawingShaders()
    {
        if (m_PreviewMode != MaskPreviewMode::Painted || !m_IsEditing) {
            FinishActiveStroke();
            return false;
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const bool hadStroke = m_HasActiveStroke;
            FinishActiveStroke();
            return hadStroke;
        }
        if (m_DrawSettings.m_BrushPositionX < 0.0f || m_DrawSettings.m_BrushPositionX > 1.0f ||
            m_DrawSettings.m_BrushPositionY < 0.0f || m_DrawSettings.m_BrushPositionY > 1.0f) {
            FinishActiveStroke();
            return false;
        }

        const glm::vec2 currentPosition(m_DrawSettings.m_BrushPositionX, m_DrawSettings.m_BrushPositionY);
        AppendActiveStrokePoint(currentPosition);
        RasterizeStrokes();
        return true;
    }

    void MaskTool::UpdateViewportOverlay(bool showBrush, bool showMask)
    {
        m_DrawSettings.m_MaskTexture     = GetPreviewTexture() != nullptr ? GetPreviewTexture()->GetRendererID() : -1;
        m_DrawSettings.m_MaskColor       = m_VizColor;
        m_DrawSettings.m_ShowMask        = showMask;
        m_DrawSettings.m_InvertMask      = m_InvertPreview;
        m_DrawSettings.m_ShowBrushCursor = showBrush;
        m_AppState->rendererManager->GetObjectRenderer()->SetCustomBaseShapeDrawSettings(&m_DrawSettings);
    }

    bool MaskTool::ShowPaintedSettings()
    {
        bool changed = false;
        if (!m_IsEditing && ImGui::Button("Edit mask")) {
            if (s_CurrentlyEditingMaskTool != nullptr) {
                s_CurrentlyEditingMaskTool->FinishActiveStroke();
                s_CurrentlyEditingMaskTool->m_IsEditing = false;
            }
            m_IsEditing                = true;
            s_CurrentlyEditingMaskTool = this;
            changed                    = true;
        }
        const bool canUndo = m_HasActiveStroke || !m_Strokes.empty();
        ImGui::BeginDisabled(!canUndo);
        if (ImGui::Button("Undo stroke"))
            changed |= UndoLastStroke();
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("Ctrl+Z");
        if (m_IsEditing && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
            changed |= UndoLastStroke();
        }

        if (m_IsEditing) {
            if (ImGui::Button("Stop editing")) {
                FinishActiveStroke();
                m_IsEditing = false;
                if (s_CurrentlyEditingMaskTool == this)
                    s_CurrentlyEditingMaskTool = nullptr;
                changed = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Drag on the terrain to paint");

            static const char *brushModes[] = {"Paint", "Erase"};
            if (ShowComboBox("Brush mode", &m_DrawSettings.m_BrushMode, brushModes, IM_ARRAYSIZE(brushModes))) {
                m_PreviousBrushMode = m_DrawSettings.m_BrushMode;
                changed             = true;
            }
            if (ImGui::SliderFloat("Brush size", &m_DrawSettings.m_BrushSize, 0.005f, 2.0f))
                changed = true;
            if (ImGui::SliderFloat("Falloff", &m_DrawSettings.m_BrushFalloff, 0.0f, 1.0f))
                changed = true;
            if (ImGui::SliderFloat("Strength", &m_DrawSettings.m_BrushStrength, 0.0f, 1.0f))
                changed = true;
            ImGui::TextDisabled("Shift+R:size     Shift+S:strength");
            ImGui::TextDisabled("Shift+F:falloff  Ctrl:erase");
        }
        return changed;
    }

    bool MaskTool::ShowSettings(bool showViewportMask)
    {
        m_RequireUpdation = false;

        ImGui::Separator();
        ImGui::Text("Mask Tool");
        ImGui::TextDisabled("Paint a reusable mask or inspect a live calculated mask.");

        int previewMode = static_cast<int>(m_PreviewMode);
        if (ImGui::RadioButton("Manual", previewMode == static_cast<int>(MaskPreviewMode::Painted))) {
            SetPreviewMode(MaskPreviewMode::Painted);
            previewMode = static_cast<int>(m_PreviewMode);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(m_ExternalGeneratedTexture == nullptr);
        if (ImGui::RadioButton("Generated", previewMode == static_cast<int>(MaskPreviewMode::Generated))) {
            SetPreviewMode(MaskPreviewMode::Generated);
            previewMode = static_cast<int>(m_PreviewMode);
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Active source: %s (used by generation)",
                            m_PreviewMode == MaskPreviewMode::Generated ? "Generated" : "Manual");

        GeneratorTexture *previewTexture = GetPreviewTexture();
        if (previewTexture != nullptr) {
            UpdateVisualizationTexture(previewTexture);
            GeneratorTexture *displayTexture = m_InvertPreview ? m_VisualizationTexture.get() : previewTexture;
            const float previewSize          = glm::clamp(ImGui::GetContentRegionAvail().x, 160.0f, 512.0f);
            ImGui::Image(displayTexture->GetTextureID(), ImVec2(previewSize, previewSize));
        }

        bool changed = false;
        if (m_PreviewMode == MaskPreviewMode::Painted) {
            changed |= ShowPaintedSettings();
        } else {
            if (m_ExternalGeneratedTexture != nullptr) {
                ImGui::TextDisabled("%s (read-only generated mask)", m_GeneratedMaskLabel.c_str());
                ImGui::TextDisabled("Generation settings belong to the source generator or filter.");
                if (ImGui::Button("Manually Edit"))
                    CopyGeneratedMaskToPainted();
            } else {
                ImGui::TextDisabled("No generated mask is attached.");
            }
        }

        ViewportManager *activeViewport = nullptr;
        for (auto viewport : m_AppState->viewportManagers) {
            if (viewport->IsActive()) {
                activeViewport = viewport;
                break;
            }
        }

        m_DrawSettings.m_BrushPositionX = m_DrawSettings.m_BrushPositionY = -1000.0f;
        bool showBrush                                                    = false;
        if (activeViewport) {
            const auto position             = activeViewport->GetPositionOnTerrain();
            m_DrawSettings.m_BrushPositionX = position.x;
            m_DrawSettings.m_BrushPositionY = position.y;

            if (m_PreviewMode == MaskPreviewMode::Painted && m_IsEditing) {
                showBrush = true;
                if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
                    if (ImGui::IsKeyDown(ImGuiKey_R)) {
                        activeViewport->SetControlEnabled(false);
                        m_DrawSettings.m_BrushSize = glm::clamp(m_DrawSettings.m_BrushSize + ImGui::GetIO().MouseWheel * 0.2f * (m_DrawSettings.m_BrushSize + 0.01f), 0.005f, 2.0f);
                    } else if (ImGui::IsKeyDown(ImGuiKey_S)) {
                        activeViewport->SetControlEnabled(false);
                        m_DrawSettings.m_BrushStrength = glm::clamp(m_DrawSettings.m_BrushStrength + ImGui::GetIO().MouseWheel * 0.01f, 0.0f, 1.0f);
                    } else if (ImGui::IsKeyDown(ImGuiKey_F)) {
                        activeViewport->SetControlEnabled(false);
                        m_DrawSettings.m_BrushFalloff = glm::clamp(m_DrawSettings.m_BrushFalloff + ImGui::GetIO().MouseWheel * 0.05f, 0.0f, 1.0f);
                    }
                } else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                    if (m_DrawSettings.m_BrushMode != 1)
                        m_PreviousBrushMode = m_DrawSettings.m_BrushMode;
                    m_DrawSettings.m_BrushMode = 1;
                } else {
                    m_DrawSettings.m_BrushMode = m_PreviousBrushMode;
                }
                ApplyDrawingShaders();
            }
        }

        // Filter settings can inspect the mask without tinting the terrain. Once the
        // user enters painted editing, the overlay becomes active for that tool.
        UpdateViewportOverlay(showBrush, showViewportMask || m_IsEditing);

        if (changed)
            m_RequireUpdation = true;
        return m_RequireUpdation;
    }

} // namespace tf3d::generators
