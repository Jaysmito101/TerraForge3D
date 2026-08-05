#include "Base/Camera.h"

#include "imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr float MinElevation = glm::radians(2.0f);
constexpr float MaxElevation = glm::radians(89.0f);
constexpr float MinDistance = 0.01f;
constexpr float MaxDistance = 1000000.0f;


Camera::Camera(bool perspective)
: m_Perspective(perspective)
{
	static int s_NextCameraID = 0;
	m_CameraID = s_NextCameraID++;
	Reset();
}

void Camera::Reset()
{
	m_Target = glm::vec3(0.0f);
	m_Distance = 3.65f;
	m_Azimuth = -9.75f;
	m_Elevation = glm::radians(57.0f);
	ClampOrbit();
	UpdateCamera();
}

void Camera::ClampOrbit()
{
	m_Distance = std::clamp(m_Distance, MinDistance, MaxDistance);
	m_Elevation = std::clamp(m_Elevation, MinElevation, MaxElevation);
	m_Azimuth = std::remainder(m_Azimuth, glm::two_pi<float>());
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
	if (viewportHeight <= 0.0f) return;
	const glm::vec3 forward = glm::normalize(m_Target - m_Position);
	const glm::vec3 right = glm::normalize(glm::cross(forward, m_WorldUp));
	const glm::vec3 up = glm::normalize(glm::cross(right, forward));
	const float worldPerPixel = 2.0f * m_Distance * std::tan(glm::radians(m_FieldOfView) * 0.5f) / viewportHeight;
	m_Target += (-deltaX * right + deltaY * up) * worldPerPixel;
}

void Camera::Zoom(float wheelDelta)
{
	if (std::abs(wheelDelta) < 0.000001f) return;
	m_Distance *= std::pow(0.85f, wheelDelta);
	ClampOrbit();
}

void Camera::RebuildProjection()
{
	float nearClip = m_NearClip;
	float farClip = m_FarClip;
	if (m_AutomaticClipping)
	{
		nearClip = std::clamp(m_Distance * 0.001f, 0.005f, 1.0f);
		farClip = std::max(1000.0f, m_Distance * 100.0f);
	}
	farClip = std::max(farClip, nearClip + 0.01f);

	if (m_Perspective) {
		m_Projection = glm::perspective(glm::radians(m_FieldOfView), m_AspectRatio, nearClip, farClip);
	}
	else
	{
		const float halfHeight = m_Distance * std::tan(glm::radians(m_FieldOfView) * 0.5f);
		m_Projection = glm::ortho(-halfHeight * m_AspectRatio, halfHeight * m_AspectRatio,
			-halfHeight, halfHeight, nearClip, farClip);
	}
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
	m_View = glm::lookAt(m_Position, m_Target, m_WorldUp);
	RebuildProjection();
	m_ProjectionView = m_Projection * m_View;
}

nlohmann::json Camera::Save() const
{
	nlohmann::json data;
	data["CameraID"] = m_CameraID;
	data["Perspective"] = m_Perspective;
	data["AutomaticClipping"] = m_AutomaticClipping;
	data["Target"] = {{"X", m_Target.x}, {"Y", m_Target.y}, {"Z", m_Target.z}};
	data["Distance"] = m_Distance;
	data["Azimuth"] = glm::degrees(m_Azimuth);
	data["Elevation"] = glm::degrees(m_Elevation);
	data["FieldOfView"] = m_FieldOfView;
	data["NearClip"] = m_NearClip;
	data["FarClip"] = m_FarClip;
	data["AspectRatio"] = m_AspectRatio;
	return data;
}

void Camera::Load(const nlohmann::json& data)
{
	m_CameraID = data.value("CameraID", m_CameraID);
	m_Perspective = data.value("Perspective", m_Perspective);
	m_AutomaticClipping = data.value("AutomaticClipping", m_AutomaticClipping);
	if (data.contains("Target"))
	{
		const auto& target = data["Target"];
		m_Target = glm::vec3(target.value("X", 0.0f), target.value("Y", 0.0f), target.value("Z", 0.0f));
	}
	m_Distance = data.value("Distance", m_Distance);
	m_Azimuth = glm::radians(data.value("Azimuth", glm::degrees(m_Azimuth)));
	m_Elevation = glm::radians(data.value("Elevation", glm::degrees(m_Elevation)));
	m_FieldOfView = data.value("FieldOfView", m_FieldOfView);
	m_NearClip = data.value("NearClip", m_NearClip);
	m_FarClip = data.value("FarClip", m_FarClip);
	m_AspectRatio = data.value("AspectRatio", m_AspectRatio);
	ClampOrbit();
	UpdateCamera();
}

void Camera::ShowSettings(bool renderWindow, bool* pOpen)
{
	if (pOpen != nullptr && !*pOpen) return;
	if (renderWindow) ImGui::Begin(("Camera Controls##" + std::to_string(m_CameraID)).c_str(), pOpen);

	ImGui::TextUnformatted("Orbit Camera");
	ImGui::DragFloat3("Target", &m_Target.x, 0.01f);
	ImGui::DragFloat("Distance", &m_Distance, 0.01f, MinDistance, MaxDistance, "%.3f");
	float azimuthDegrees = glm::degrees(m_Azimuth);
	float elevationDegrees = glm::degrees(m_Elevation);
	if (ImGui::DragFloat("Azimuth", &azimuthDegrees, 0.25f)) m_Azimuth = glm::radians(azimuthDegrees);
	if (ImGui::DragFloat("Elevation", &elevationDegrees, 0.25f, glm::degrees(MinElevation), glm::degrees(MaxElevation))) m_Elevation = glm::radians(elevationDegrees);
	ImGui::DragFloat("Field of View", &m_FieldOfView, 0.1f, 10.0f, 120.0f);
	ImGui::Checkbox("Perspective", &m_Perspective);
	ImGui::Checkbox("Automatic Clipping", &m_AutomaticClipping);
	if (!m_AutomaticClipping)
	{
		ImGui::DragFloat("Near Clip", &m_NearClip, 0.001f, 0.0001f, 1000.0f, "%.4f");
		ImGui::DragFloat("Far Clip", &m_FarClip, 1.0f, 1.0f, MaxDistance, "%.1f");
	}
	if (ImGui::Button("Reset Camera")) Reset();
	ClampOrbit();

	if (renderWindow) ImGui::End();
}
