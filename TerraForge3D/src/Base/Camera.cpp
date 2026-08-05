#include "Base/Camera.h"
#include "Exporters/Serializer.h"

#include "imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr float MinElevation = glm::radians(2.0f);
constexpr float MaxElevation = glm::radians(89.0f);
constexpr float MinDistance  = 0.01f;
constexpr float MaxDistance  = 1000000.0f;

Camera::Camera(bool perspective)
    : m_Perspective(perspective)
{
    static int s_NextCameraID = 0;
    m_CameraID                = s_NextCameraID++;
    Reset();
}

void Camera::Reset()
{
    m_Target    = glm::vec3(0.0f);
    m_Distance  = 3.65f;
    m_Azimuth   = -9.75f;
    m_Elevation = glm::radians(57.0f);
    ClampOrbit();
    UpdateCamera();
}

void Camera::ClampOrbit()
{
    m_Distance  = std::clamp(m_Distance, MinDistance, MaxDistance);
    m_Elevation = std::clamp(m_Elevation, MinElevation, MaxElevation);
    m_Azimuth   = std::remainder(m_Azimuth, glm::two_pi<float>());
}

void Camera::SetAspectRatio(float aspectRatio)
{
    m_AspectRatio = std::max(std::abs(aspectRatio), 0.001f);
}

void Camera::Orbit(float deltaX, float deltaY, float sensitivity)
{
    m_Azimuth -= deltaX * sensitivity;
    m_Elevation -= deltaY * sensitivity;
    ClampOrbit();
}

void Camera::Pan(float deltaX, float deltaY, float viewportHeight)
{
    if (viewportHeight <= 0.0f)
        return;
    const glm::vec3 forward   = glm::normalize(m_Target - m_Position);
    const glm::vec3 right     = glm::normalize(glm::cross(forward, m_WorldUp));
    const glm::vec3 up        = glm::normalize(glm::cross(right, forward));
    const float worldPerPixel = 2.0f * m_Distance * std::tan(glm::radians(m_FieldOfView) * 0.5f) / viewportHeight;
    m_Target += (-deltaX * right + deltaY * up) * worldPerPixel;
}

void Camera::Zoom(float wheelDelta)
{
    if (std::abs(wheelDelta) < 0.000001f)
        return;
    m_Distance *= std::pow(0.85f, wheelDelta);
    ClampOrbit();
}

void Camera::RebuildProjection()
{
    float nearClip = GetEffectiveNearClip();
    float farClip  = GetEffectiveFarClip();
    farClip        = std::max(farClip, nearClip + 0.01f);

    if (m_Perspective) {
        m_Projection = glm::perspective(glm::radians(m_FieldOfView), m_AspectRatio, nearClip, farClip);
    } else {
        const float halfHeight = m_Distance * std::tan(glm::radians(m_FieldOfView) * 0.5f);
        m_Projection           = glm::ortho(-halfHeight * m_AspectRatio, halfHeight * m_AspectRatio,
                                            -halfHeight, halfHeight, nearClip, farClip);
    }
}

float Camera::GetEffectiveNearClip() const
{
    return m_AutomaticClipping ? std::clamp(m_Distance * 0.001f, 0.005f, 1.0f) : m_NearClip;
}

float Camera::GetEffectiveFarClip() const
{
    return m_AutomaticClipping ? std::max(1000.0f, m_Distance * 100.0f) : m_FarClip;
}

void Camera::UpdateCamera()
{
    ClampOrbit();
    const float cosElevation = std::cos(m_Elevation);
    const glm::vec3 orbitDirection(
        std::sin(m_Azimuth) * cosElevation,
        std::sin(m_Elevation),
        std::cos(m_Azimuth) * cosElevation);
    m_Position = m_Target + orbitDirection * m_Distance;
    m_View     = glm::lookAt(m_Position, m_Target, m_WorldUp);
    RebuildProjection();
    m_ProjectionView = m_Projection * m_View;
}

std::shared_ptr<SerializerNodeInternal> Camera::Save() const
{
    SerializerNode node = CreateSerializerNode();
    node->SetInteger("CameraID", m_CameraID);
    node->SetInteger("Perspective", m_Perspective ? 1 : 0);
    node->SetInteger("AutomaticClipping", m_AutomaticClipping ? 1 : 0);
    node->SetFloat("TargetX", m_Target.x);
    node->SetFloat("TargetY", m_Target.y);
    node->SetFloat("TargetZ", m_Target.z);
    node->SetFloat("Distance", m_Distance);
    node->SetFloat("Azimuth", glm::degrees(m_Azimuth));
    node->SetFloat("Elevation", glm::degrees(m_Elevation));
    node->SetFloat("FieldOfView", m_FieldOfView);
    node->SetFloat("NearClip", m_NearClip);
    node->SetFloat("FarClip", m_FarClip);
    node->SetFloat("AspectRatio", m_AspectRatio);
    return node;
}

void Camera::Load(std::shared_ptr<SerializerNodeInternal> data)
{
    if (!data)
        return;
    m_CameraID          = data->GetInteger("CameraID", m_CameraID);
    m_Perspective       = data->GetInteger("Perspective", m_Perspective ? 1 : 0) != 0;
    m_AutomaticClipping = data->GetInteger("AutomaticClipping", m_AutomaticClipping ? 1 : 0) != 0;
    m_Target            = glm::vec3(
        data->GetFloat("TargetX", m_Target.x),
        data->GetFloat("TargetY", m_Target.y),
        data->GetFloat("TargetZ", m_Target.z));
    m_Distance    = data->GetFloat("Distance", m_Distance);
    m_Azimuth     = glm::radians(data->GetFloat("Azimuth", glm::degrees(m_Azimuth)));
    m_Elevation   = glm::radians(data->GetFloat("Elevation", glm::degrees(m_Elevation)));
    m_FieldOfView = data->GetFloat("FieldOfView", m_FieldOfView);
    m_NearClip    = data->GetFloat("NearClip", m_NearClip);
    m_FarClip     = data->GetFloat("FarClip", m_FarClip);
    m_AspectRatio = data->GetFloat("AspectRatio", m_AspectRatio);
    ClampOrbit();
    UpdateCamera();
}

void Camera::ShowSettings(bool renderWindow, bool *pOpen)
{
    if (pOpen != nullptr && !*pOpen)
        return;
    if (renderWindow)
        ImGui::Begin(("Camera Controls##" + std::to_string(m_CameraID)).c_str(), pOpen);

    ImGui::TextUnformatted("Orbit Camera");
    ImGui::DragFloat3("Target", &m_Target.x, 0.01f);
    ImGui::DragFloat("Distance", &m_Distance, 0.01f, MinDistance, MaxDistance, "%.3f");
    float azimuthDegrees   = glm::degrees(m_Azimuth);
    float elevationDegrees = glm::degrees(m_Elevation);
    if (ImGui::DragFloat("Azimuth", &azimuthDegrees, 0.25f))
        m_Azimuth = glm::radians(azimuthDegrees);
    if (ImGui::DragFloat("Elevation", &elevationDegrees, 0.25f, glm::degrees(MinElevation), glm::degrees(MaxElevation)))
        m_Elevation = glm::radians(elevationDegrees);
    ImGui::DragFloat("Field of View", &m_FieldOfView, 0.1f, 10.0f, 120.0f);
    ImGui::Checkbox("Perspective", &m_Perspective);
    ImGui::Checkbox("Automatic Clipping", &m_AutomaticClipping);
    if (!m_AutomaticClipping) {
        ImGui::DragFloat("Near Clip", &m_NearClip, 0.001f, 0.0001f, 1000.0f, "%.4f");
        ImGui::DragFloat("Far Clip", &m_FarClip, 1.0f, 1.0f, MaxDistance, "%.1f");
    }
    if (ImGui::Button("Reset Camera"))
        Reset();
    ClampOrbit();

    if (renderWindow)
        ImGui::End();
}
