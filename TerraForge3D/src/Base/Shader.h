#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

class Shader
{
public:
	Shader(std::string vertexStc, std::string fragmentSrc, std::string geometrySource);
	Shader(std::string vertexStc, std::string fragmentSrc);

	~Shader();

	void Bind();

	void SetLightPos(glm::vec3 &);
	void SetLightCol(float *);
	void SetTime(float *);
	void SetMPV(glm::mat4 &);

	void SetUniformf(std::string name, float value);
	void SetUniform3f(std::string name, float *value);
	void SetUniformi(std::string name, int value);
	void SetUniformMat4(std::string name, glm::mat4 value);

	void Unbind();

	inline int GetNativeShader()
	{
		return m_Shader;
	}

	int m_Shader=0, m_UniformId=0, m_LightPosUniformID=0, m_LightColUniformID=0, m_TimeUniformID=0;
	std::unordered_map<std::string, uint32_t> uniformLocations;

};