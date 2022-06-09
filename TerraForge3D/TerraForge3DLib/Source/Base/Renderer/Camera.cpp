#include "Base/Renderer/Camera.hpp"

namespace TerraForge3D
{

	namespace RendererAPI
	{

		Camera::Camera()
		{
		}

		Camera::~Camera()
		{
		}

		void Camera::RecalculateMatrices()
		{
			glm::mat4 rotM = glm::mat4(1.0f);
			glm::mat4 transM = glm::mat4(1.0f);
			
			rotM = glm::rotate(rotM, glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
			rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			
			glm::vec3 translation = position;
			if (flipY)
				translation.y *= -1.0f;
			transM = glm::translate(transM, translation);

			matrices.view = transM * rotM;

			viewPos.x = position.x;
			viewPos.y = position.y;
			viewPos.z = position.z;
			viewPos.w = 0.0f;
			viewPos = viewPos * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

			if (projectionMode == CameraProjection_Perspective)
			{
				TF3D_ASSERT(aspect != 0.0f, "Aspect ration cannot be 0");
				matrices.projection = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
			}
			else if (projectionMode == CameraProjection_Orthographic)
			{
				matrices.projection = glm::ortho(orthoViewportSize.x, orthoViewportSize.y, orthoViewportSize.z, orthoViewportSize.w);
			}
			if (flipY)
				matrices.projection[1][1] *= -1.0f;
			matrices.pv = matrices.projection * matrices.view;
		}

		Camera* Camera::SetPerspective(float fov, float aspect, float zNear, float zFar)
		{
			this->fov = fov;
			this->zNear = zNear;
			this->zFar = zFar;
			this->aspect = aspect;
			this->projectionMode = CameraProjection_Perspective;
			RecalculateMatrices();
			return this;
		}

		Camera* Camera::SetOrthographic(float left, float right, float top, float bottom)
		{
			this->orthoViewportSize.x = left;
			this->orthoViewportSize.y = right;
			this->orthoViewportSize.z = bottom;
			this->orthoViewportSize.w = top;
			RecalculateMatrices();
			return this;
		}

	}

}