#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>

class ComputeShader
{
public:
	ComputeShader(std::string source);
	~ComputeShader();
	inline void Bind() { glUseProgram(m_Shader); }
	inline void Unbind() { glUseProgram(0); }
	inline void Dispatch(int x, int y, int z) { glDispatchCompute(x, y, z); }
	inline void SetMemoryBarrier() { glMemoryBarrier(GL_ALL_BARRIER_BITS); }
	inline int GetUniformLocation(const std::string& name)
	{
		if (m_UniformIds.find(name) != m_UniformIds.end()) return m_UniformIds[name];
		auto location = glGetUniformLocation(m_Shader, name.c_str());
		m_UniformIds[name] = location;
		return location;
	}
	void SetUniform1i(const std::string& name, int value) { glUniform1i(GetUniformLocation(name), value); }
	void SetUniform1f(const std::string& name, float value) { glUniform1f(GetUniformLocation(name), value); }
	void SetUniform2f(const std::string& name, const glm::vec2& value) { glUniform2f(GetUniformLocation(name), value.x, value.y); }
	void SetUniform3f(const std::string& name, const glm::vec3& value) { glUniform3f(GetUniformLocation(name), value.x, value.y, value.z); }
	void SetUniform4f(const std::string& name, const glm::vec4& value) { glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w); }
	void SetUniformMat4f(const std::string& name, const glm::mat4& matrix) { glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]); }
	inline int GetNativeShader() { return m_Shader; }
private:
	int m_Shader;
	std::unordered_map<std::string, uint32_t> m_UniformIds;
	int maxWorkGroupCount[3], maxWorkGroupSize[3], maxWorkGroupInvocations;
};