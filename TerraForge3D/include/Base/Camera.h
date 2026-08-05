#pragma once

#include <glm/glm.hpp>
#include <memory>

class SerializerNodeInternal;

class Camera
{
public:
	 explicit Camera(bool perspective = true);

	void UpdateCamera();
	void SetAspectRatio(float aspectRatio);
	void Orbit(float deltaX, float deltaY, float sensitivity = 0.005f);
	void Pan(float deltaX, float deltaY, float viewportHeight);
	void Zoom(float wheelDelta);
	void Reset();

	std::shared_ptr<SerializerNodeInternal> Save() const;
	void Load(std::shared_ptr<SerializerNodeInternal> data);
	void ShowSettings(bool renderWindow = false, bool* pOpen = nullptr);

	const glm::mat4& GetViewMatrix() const { return m_View; }
	const glm::mat4& GetProjectionMatrix() const { return m_Projection; }
	const glm::mat4& GetProjectionViewMatrix() const { return m_ProjectionView; }
	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetTarget() const { return m_Target; }

	float GetFieldOfView() const { return m_FieldOfView; }
	float GetNearClip() const { return m_NearClip; }
	float GetFarClip() const { return m_FarClip; }
	bool IsPerspective() const { return m_Perspective; }

private:
	void RebuildProjection();
	void ClampOrbit();

	glm::mat4 m_View = glm::mat4(1.0f);
	glm::mat4 m_Projection = glm::mat4(1.0f);
	glm::mat4 m_ProjectionView = glm::mat4(1.0f);
	glm::vec3 m_Position = glm::vec3(0.0f, 1.5f, 3.1f);
	glm::vec3 m_Target = glm::vec3(0.0f);
	glm::vec3 m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	float m_Distance = 3.1f;
	float m_Azimuth = 0.0f;
	float m_Elevation = glm::radians(28.0f);
	float m_FieldOfView = 45.0f;
	float m_NearClip = 0.01f;
	float m_FarClip = 10000.0f;
	float m_AspectRatio = 16.0f / 9.0f;
	bool m_Perspective = true;
	bool m_AutomaticClipping = true;
	int m_CameraID = 0;
};
