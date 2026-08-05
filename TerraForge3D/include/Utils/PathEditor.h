#pragma once

#include "Base/Base.h"

namespace TerraForge3D::UI
{
    template <size_t MaxPoints>
    bool DrawPathEditor(const char *label, std::array<glm::vec2, MaxPoints> &points, int &pointCount,
                        int minimumPoints = 2)
    {
        const int minimum = std::clamp(minimumPoints, 1, static_cast<int>(MaxPoints));
        pointCount        = std::clamp(pointCount, minimum, static_cast<int>(MaxPoints));
        bool changed      = false;

        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        ImGui::TextDisabled("Drag handles. Double-click empty space to add; double-click a handle to remove.");

        const float width      = std::max(ImGui::GetContentRegionAvail().x, 160.0f);
        const float height     = width;
        const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize(width, height);
        ImGui::InvisibleButton("PathCanvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
        const ImVec2 canvasMax(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);
        ImDrawList *drawList    = ImGui::GetWindowDrawList();
        const ImU32 background  = ImGui::GetColorU32(ImGuiCol_FrameBg);
        const ImU32 grid        = ImGui::GetColorU32(ImGuiCol_Border);
        const ImU32 line        = ImGui::GetColorU32(ImGuiCol_PlotLines);
        const ImU32 point       = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        const ImU32 activePoint = ImGui::GetColorU32(ImGuiCol_ButtonActive);
        drawList->AddRectFilled(canvasMin, canvasMax, background, 4.0f);
        for (int gridIndex = 1; gridIndex < 4; ++gridIndex) {
            const float x = canvasMin.x + canvasSize.x * (static_cast<float>(gridIndex) / 4.0f);
            const float y = canvasMin.y + canvasSize.y * (static_cast<float>(gridIndex) / 4.0f);
            drawList->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMax.y), grid, 1.0f);
            drawList->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), grid, 1.0f);
        }
        drawList->AddRect(canvasMin, canvasMax, grid, 4.0f);

        auto toScreen = [&](const glm::vec2 &value) {
            return ImVec2(canvasMin.x + value.x * canvasSize.x,
                          canvasMax.y - value.y * canvasSize.y);
        };
        auto toNormalized = [&](const ImVec2 &value) {
            return glm::clamp(glm::vec2(
                                  (value.x - canvasMin.x) / std::max(canvasSize.x, 1.0f),
                                  (canvasMax.y - value.y) / std::max(canvasSize.y, 1.0f)),
                              glm::vec2(0.0f), glm::vec2(1.0f));
        };

        for (int pointIndex = 1; pointIndex < pointCount; ++pointIndex)
            drawList->AddLine(toScreen(points[pointIndex - 1]), toScreen(points[pointIndex]), line, 2.0f);

        ImGuiStorage *state          = ImGui::GetStateStorage();
        const ImGuiID activePointKey = ImGui::GetID("ActivePathPoint");
        int *draggedPoint            = state->GetIntRef(activePointKey, -1);
        const bool hovered           = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            const ImVec2 mouse           = ImGui::GetIO().MousePos;
            int closestPoint             = -1;
            float closestDistanceSquared = 10.0f * 10.0f;
            for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                const ImVec2 screenPoint    = toScreen(points[pointIndex]);
                const float dx              = screenPoint.x - mouse.x;
                const float dy              = screenPoint.y - mouse.y;
                const float distanceSquared = dx * dx + dy * dy;
                if (distanceSquared <= closestDistanceSquared) {
                    closestDistanceSquared = distanceSquared;
                    closestPoint           = pointIndex;
                }
            }
            if (closestPoint >= 0 && pointCount > minimum) {
                for (int pointIndex = closestPoint; pointIndex + 1 < pointCount; ++pointIndex)
                    points[pointIndex] = points[pointIndex + 1];
                --pointCount;
                *draggedPoint = -1;
                changed       = true;
            } else if (closestPoint < 0 && pointCount < static_cast<int>(MaxPoints)) {
                points[pointCount] = toNormalized(mouse);
                ++pointCount;
                changed = true;
            }
        } else if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 mouse           = ImGui::GetIO().MousePos;
            float closestDistanceSquared = 10.0f * 10.0f;
            *draggedPoint                = -1;
            for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                const ImVec2 screenPoint    = toScreen(points[pointIndex]);
                const float dx              = screenPoint.x - mouse.x;
                const float dy              = screenPoint.y - mouse.y;
                const float distanceSquared = dx * dx + dy * dy;
                if (distanceSquared <= closestDistanceSquared) {
                    closestDistanceSquared = distanceSquared;
                    *draggedPoint          = pointIndex;
                }
            }
        }
        if (*draggedPoint >= 0 && *draggedPoint < pointCount && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const glm::vec2 nextPosition = toNormalized(ImGui::GetIO().MousePos);
            if (points[*draggedPoint] != nextPosition) {
                points[*draggedPoint] = nextPosition;
                changed               = true;
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            *draggedPoint = -1;

        for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            const ImVec2 screenPoint = toScreen(points[pointIndex]);
            const ImU32 color        = pointIndex == *draggedPoint ? activePoint : point;
            drawList->AddCircleFilled(screenPoint, pointIndex == *draggedPoint ? 7.0f : 6.0f, color);
            drawList->AddCircle(screenPoint, 7.0f, ImGui::GetColorU32(ImGuiCol_Text));
            drawList->AddText(ImVec2(screenPoint.x + 9.0f, screenPoint.y - 8.0f), ImGui::GetColorU32(ImGuiCol_Text),
                              std::to_string(pointIndex + 1).c_str());
        }

        if (ImGui::Button("Add point")) {
            if (pointCount < static_cast<int>(MaxPoints)) {
                const int previous = std::max(pointCount - 1, 0);
                points[pointCount] = glm::clamp(points[previous] + glm::vec2(0.05f, 0.05f),
                                                glm::vec2(0.0f), glm::vec2(1.0f));
                ++pointCount;
                changed = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove last") && pointCount > minimum) {
            --pointCount;
            changed = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%d / %d points", pointCount, static_cast<int>(MaxPoints));
        ImGui::PopID();
        return changed;
    }
} // namespace TerraForge3D::UI
