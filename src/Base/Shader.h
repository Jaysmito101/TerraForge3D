#pragma once

#include <glm/glm.hpp>

class Shader
{
public:
	Shader(std::string vertexStc, std::string fragmentSrc);

	~Shader();

	void Bind();

	void SetLightPos(glm::vec3);
	void SetLightCol(float*);
	void SetTime(float*);
	void SetMPV(glm::mat4);

	void Unbind();

	inline int GetNativeShader() { return m_Shader; }

private:
	int m_Shader, m_UniformId, m_LightPosUniformID, m_LightColUniformID, m_TimeUniformID;
};