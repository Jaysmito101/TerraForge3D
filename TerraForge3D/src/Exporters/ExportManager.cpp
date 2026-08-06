#include "Exporters/ExportManager.h"
#include "Data/ApplicationState.h"
#include "Utils/Utils.h"

#include <fstream>
#include <sstream>

namespace tf3d::exporters
{

    ExportManager::ExportManager(ApplicationState *as)
    {
        m_AppState        = as;
        m_VisualzeTexture = std::make_shared<GeneratorTexture>(256, 256);
        m_VisualzeShader  = m_AppState->resourceManager->LoadComputeShader("exporters/texture_export_visualizer", true);
    }

    ExportManager::~ExportManager()
    {
    }

    void ExportManager::ShowSettings()
    {
        if (!m_IsWindowOpen)
            return;
        ImGui::Begin("Export Manager##RootWindow", &m_IsWindowOpen);

        if (m_ExportProgress > 0.0f || m_HideExportControls) {
            if (m_StatusMessage.size() > 0)
                ImGui::Text("%s", m_StatusMessage.data());
            ImGui::ProgressBar(m_ExportProgress);
        } else {
            if (ImGui::BeginTabBar("Export Type")) {
                if (ImGui::BeginTabItem("Mesh")) {
                    ShowMeshExportSettings();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Texture")) {
                    ShowTextureExportSettings();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }

        ImGui::End();
    }

    void ExportManager::Update()
    {
        if (m_ExportProgress > 1.0f)
            m_ExportProgress = 0.0f;
    }

    void ExportManager::ShowMeshExportSettings()
    {
        static const char *exportTypes[] = {
            "WaveFont OBJ",
            "STL ASCII",
            "STL Binary",
            "PLY ASCII",
            "PLY Binary",
            "Collada",
            "GLTF"};

        if (ImGui::BeginCombo("Export Format", exportTypes[m_ExportMeshFormat])) {
            for (int i = 0; i < IM_ARRAYSIZE(exportTypes); i++) {
                bool is_selected = (m_ExportMeshFormat == i);
                if (ImGui::Selectable(exportTypes[i], is_selected))
                    m_ExportMeshFormat = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Export Current Tile")) {
            std::string output_file_path = ShowSaveFileDialog("*.*");
            if (output_file_path.size() < 3)
                return;
            ExportMeshCurrentTile(output_file_path, nullptr, m_ExportMeshFormat);
        }

        ImGui::BeginDisabled();
        if (ImGui::Button("Export All Tiles")) {
            std::string output_file_path = ShowSaveFileDialog("*.*");
            if (output_file_path.size() < 3)
                return;
            ExportMeshAllTiles(output_file_path, nullptr, m_ExportMeshFormat);
        }
        ImGui::EndDisabled();
    }

    void ExportManager::ShowTextureExportSettings()
    {
        static const char *exportTypes[] = {
            "PNG",
            "WEBP",
            "RAW",
            "EXR"};

        static const char *exportBitDepths[] = {
            "8-bit",
            "16-bit",
            "32-bit"};

        if (ImGui::BeginCombo("Export Format", exportTypes[m_ExportTextureFormat])) {
            for (int i = 0; i < IM_ARRAYSIZE(exportTypes); i++) {
                bool is_selected = (m_ExportTextureFormat == i);
                if (ImGui::Selectable(exportTypes[i], is_selected))
                    m_ExportTextureFormat = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Bit Depth", exportBitDepths[m_ExportTextureBitDepth])) {
            for (int i = 0; i < IM_ARRAYSIZE(exportBitDepths); i++) {
                bool is_selected = (m_ExportTextureBitDepth == i);
                if (ImGui::Selectable(exportBitDepths[i], is_selected))
                    m_ExportTextureBitDepth = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        const auto &fieldStatistics = m_AppState->generationManager->GetFieldStatisticsResult();
        if (fieldStatistics.valid)
            ImGui::Text("Final field range: %.4f to %.4f", fieldStatistics.minimum, fieldStatistics.maximum);
        else
            ImGui::TextUnformatted("Final field range unavailable");

        UpdateHeightmapVisualizer();
        ImGui::Image(m_VisualzeTexture->GetTextureID(), ImVec2(350, 350));

        if (ImGui::Button("Export Current Tile")) {
            std::string output_file_path = ShowSaveFileDialog("*.*");
            if (output_file_path.size() < 3)
                return;
            ExportTextureCurrentTile(output_file_path, m_ExportTextureFormat, (int)pow(2, m_ExportTextureBitDepth) * 8, nullptr);
        }

        ImGui::BeginDisabled();
        if (ImGui::Button("Export All Tiles")) {
            std::string output_file_path = ShowSaveFileDialog("*.*");
            if (output_file_path.size() < 3)
                return;
            // ExportTextureAllTiles(output_file_path, nullptr, exportTextureFormat, exportTextureBitDepth);
        }
        ImGui::EndDisabled();
    }

} // namespace tf3d::exporters
