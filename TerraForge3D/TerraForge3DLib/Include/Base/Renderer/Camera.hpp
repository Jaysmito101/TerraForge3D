#pragma once
#include "Base/Core/Core.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_INLINE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>



namespace TerraForge3D
{

	namespace RendererAPI
	{
		enum CameraProjection
		{
			CameraProjection_Perspective = 0,
			CameraProjection_Orthographic
		};

		class Camera
		{
		public:

			Camera();
			~Camera();

			void RecalculateMatrices();

			Camera* SetPerspective(float fov, float aspect, float zNear, float zFar);
			Camera* SetOrthographic(float left, float right, float top, float bottom);

			inline Camera* SetProjectionMode(CameraProjection proj) { this->projectionMode = proj; return this; }
			inline Camera* SetPosition(float x, float y, float z = 0.0f) { this->position.x = x; this->position.y = y; this->position.z = z; this->position.w = 0.0f; return this; };
			inline Camera* SetRotation(float x, float y, float z = 0.0f) { this->rotation.x = x; this->rotation.y = y; this->rotation.z = z; this->rotation.w = 0.0f; return this; };

			inline float GetNearClip() { return zNear; }
			inline float GetFarClip() { return zFar; }

		public:
			CameraProjection projectionMode = CameraProjection_Perspective;

			float zNear = 0.0f;
			float zFar = 0.0f;
			float fov = 0.0f;
			float aspect = 1.0f;

			glm::vec4 orthoViewportSize = glm::vec4(-1.0f, 1.0f, 1.0f, -1.0f);

			struct {
				glm::mat4 view = glm::mat4(1);
				glm::mat4 projection = glm::mat4(1);
				glm::mat4 pv = glm::mat4(1);
			} matrices;

			bool isPerspective = true;
			bool flipY = false;

			glm::vec4 position = glm::vec4(0.0f);
			glm::vec4 rotation = glm::vec4(0.0f);
			glm::vec4 viewPos = glm::vec4(0.0f);

		};
	}
}