#include "Generators/Masks/MaskTool.h"

#include "Data/ApplicationState.h"
#include "Generators/GeneratorTexture.h"
#include "Renderer/ObjectRenderer.h"
#include "UI/ImGuiComponents.h"
#include "Utils/Utils.h"

#include <cmath>
#include <utility>

namespace tf3d::generators
{

    MaskTool *MaskTool::s_CurrentlyEditingMaskTool = nullptr;

    MaskTool::MaskTool(tf3d::data::ApplicationState *state, glm::vec3 vizColor)
        : m_AppState(state), m_VizColor(vizColor)
    {
    }

    MaskTool::~MaskTool()
    {
        if (s_CurrentlyEditingMaskTool == this)
            s_CurrentlyEditingMaskTool = nullptr;
    }

    MaskTool::MaskTool(MaskTool &&other) noexcept
        : m_AppState(other.m_AppState),
          m_Strokes(std::move(other.m_Strokes)),
          m_ActiveStroke(std::move(other.m_ActiveStroke)),
          m_HasActiveStroke(other.m_HasActiveStroke),
          m_VizColor(other.m_VizColor),
          m_InvertPreview(other.m_InvertPreview),
          m_DrawSettings(other.m_DrawSettings),
          m_RequireUpdation(other.m_RequireUpdation),
          m_IsEditing(other.m_IsEditing),
          m_PreviousBrushMode(other.m_PreviousBrushMode)
    {
        if (s_CurrentlyEditingMaskTool == &other)
            s_CurrentlyEditingMaskTool = this;
        other.m_AppState        = nullptr;
        other.m_HasActiveStroke = false;
        other.m_IsEditing       = false;
    }

    MaskTool &MaskTool::operator=(MaskTool &&other) noexcept
    {
        if (this == &other)
            return *this;

        if (s_CurrentlyEditingMaskTool == this)
            s_CurrentlyEditingMaskTool = nullptr;

        m_AppState          = other.m_AppState;
        m_Strokes           = std::move(other.m_Strokes);
        m_ActiveStroke      = std::move(other.m_ActiveStroke);
        m_HasActiveStroke   = other.m_HasActiveStroke;
        m_VizColor          = other.m_VizColor;
        m_InvertPreview     = other.m_InvertPreview;
        m_DrawSettings      = other.m_DrawSettings;
        m_RequireUpdation   = other.m_RequireUpdation;
        m_IsEditing         = other.m_IsEditing;
        m_PreviousBrushMode = other.m_PreviousBrushMode;

        if (s_CurrentlyEditingMaskTool == &other)
            s_CurrentlyEditingMaskTool = this;
        other.m_AppState        = nullptr;
        other.m_HasActiveStroke = false;
        other.m_IsEditing       = false;
        return *this;
    }

    void MaskTool::OnResolutionChanged()
    {
        FinishActiveStroke();
        m_RequireUpdation = true;
    }

    SerializerNode MaskTool::Save() const
    {
        auto node = CreateSerializerNode();
        node->Set("InvertPreview", m_InvertPreview);

        std::vector<SerializerNode> strokes;
        strokes.reserve(m_Strokes.size() + (m_HasActiveStroke ? 1 : 0));
        const auto appendStroke = [&strokes](const MaskStroke &stroke) {
            auto strokeNode = CreateSerializerNode();
            strokeNode->Set("Points", stroke.points);
            strokeNode->Set("Strength", stroke.strength);
            strokeNode->Set("Size", stroke.size);
            strokeNode->Set("Falloff", stroke.falloff);
            strokeNode->Set("Mode", stroke.mode);
            strokes.push_back(std::move(strokeNode));
        };
        for (const auto &stroke : m_Strokes)
            appendStroke(stroke);
        if (m_HasActiveStroke)
            appendStroke(m_ActiveStroke);
        node->Set("Strokes", strokes);
        return node;
    }

    void MaskTool::Load(SerializerNode data)
    {
        if (data == nullptr)
            return;

        FinishActiveStroke();
        m_Strokes.clear();
        for (const auto &strokeNode : data->Get<std::vector<SerializerNode>>("Strokes")) {
            if (strokeNode == nullptr)
                continue;
            MaskStroke stroke;
            stroke.points   = strokeNode->Get<std::vector<glm::vec2>>("Points");
            stroke.strength = strokeNode->Get<float>("Strength", 0.0f);
            stroke.size     = strokeNode->Get<float>("Size", 0.0f);
            stroke.falloff  = strokeNode->Get<float>("Falloff", 0.0f);
            stroke.mode     = strokeNode->Get<int>("Mode", 0);
            if (!stroke.points.empty())
                m_Strokes.push_back(std::move(stroke));
        }

        m_InvertPreview = data->Get<bool>("InvertPreview", m_InvertPreview);
        m_IsEditing     = false;
        if (s_CurrentlyEditingMaskTool == this)
            s_CurrentlyEditingMaskTool = nullptr;
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
        m_RequireUpdation = true;
    }

    bool MaskTool::UndoLastStroke()
    {
        if (m_HasActiveStroke) {
            m_ActiveStroke    = MaskStroke{};
            m_HasActiveStroke = false;
            m_RequireUpdation = true;
            return true;
        }
        if (m_Strokes.empty())
            return false;

        m_Strokes.pop_back();
        m_RequireUpdation = true;
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

    void MaskTool::AppendActiveStrokePoint(const glm::vec2 &position, int resolution)
    {
        if (!m_HasActiveStroke) {
            StartActiveStroke(position);
            return;
        }

        const float sampleSpacing = glm::max(0.5f / static_cast<float>(glm::max(resolution, 1)), 0.0005f);
        if (glm::length(position - m_ActiveStroke.points.back()) >= sampleSpacing)
            m_ActiveStroke.points.push_back(position);
    }

    bool MaskTool::ApplyDrawing(int resolution)
    {
        if (!m_IsEditing) {
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
        AppendActiveStrokePoint(currentPosition, resolution);
        m_RequireUpdation = true;
        return true;
    }

    void MaskTool::UpdateViewportOverlay(GeneratorTexture *maskTexture, bool showBrush, bool showMask)
    {
        if (m_AppState == nullptr || m_AppState->rendererManager == nullptr)
            return;

        m_DrawSettings.m_MaskTexture     = maskTexture != nullptr ? maskTexture->GetRendererID() : -1;
        m_DrawSettings.m_MaskColor       = m_VizColor;
        m_DrawSettings.m_ShowMask        = showMask;
        m_DrawSettings.m_InvertMask      = m_InvertPreview;
        m_DrawSettings.m_ShowBrushCursor = showBrush;
        m_AppState->rendererManager->GetObjectRenderer()->SetDrawBrushSettings(&m_DrawSettings);
    }

    bool MaskTool::ShowStrokeSettings(int resolution)
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
        if (m_IsEditing && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
            changed |= UndoLastStroke();

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

    bool MaskTool::ShowSettings(GeneratorTexture *maskTexture,
                                GeneratorTexture *previewTexture,
                                bool showViewportMask)
    {
        m_RequireUpdation = false;

        ImGui::Separator();
        ImGui::Text("Mask Tool");
        ImGui::TextDisabled("Paint strokes on top of the selected mask base.");

        if (previewTexture != nullptr) {
            const float previewSize = glm::clamp(ImGui::GetContentRegionAvail().x, 160.0f, 512.0f);
            ImGui::Image(previewTexture->GetTextureID(), ImVec2(previewSize, previewSize));
        }

        const int resolution = maskTexture != nullptr ? glm::max(maskTexture->GetWidth(), 1) : 1;
        bool changed         = ShowStrokeSettings(resolution);

        misc::ViewportManager *activeViewport = nullptr;
        if (m_AppState != nullptr) {
            for (auto viewport : m_AppState->viewportManagers) {
                if (viewport->IsActive()) {
                    activeViewport = viewport;
                    break;
                }
            }
        }

        m_DrawSettings.m_BrushPositionX = m_DrawSettings.m_BrushPositionY = -1000.0f;
        bool showBrush                                                    = false;
        if (activeViewport) {
            const auto position             = activeViewport->GetPositionOnTerrain();
            m_DrawSettings.m_BrushPositionX = position.x;
            m_DrawSettings.m_BrushPositionY = position.y;

            if (m_IsEditing) {
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
                changed |= ApplyDrawing(resolution);
            }
        }

        UpdateViewportOverlay(maskTexture, showBrush, showViewportMask || m_IsEditing);

        if (changed)
            m_RequireUpdation = true;
        return m_RequireUpdation;
    }

} // namespace tf3d::generators
