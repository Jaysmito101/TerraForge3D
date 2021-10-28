#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

class ComputeShader
{
public:
	ComputeShader(std::string source);

	~ComputeShader();

	void Bind();

	void SetUniformf(std::string name, float value);
	void SetUniform3f(std::string name, float* value);
	void SetUniformi(std::string name, int value);
	void SetUniformMat4(std::string name, glm::mat4& value);
	void SetVec3(std::string name, float a, float b, float c);

	void Unbind();

	void Dispatch(int x, int y, int z);
	void SetMemoryBarrier();

	inline int GetNativeShader() { return m_Shader; }

	int m_Shader, m_UniformId, m_LightPosUniformID, m_LightColUniformID, m_TimeUniformID;
	std::unordered_map<std::string, uint32_t> uniformLocations;
	int maxWorkGroupCount[3], maxWorkGroupSize[3], maxWorkGroupInvocations;
};