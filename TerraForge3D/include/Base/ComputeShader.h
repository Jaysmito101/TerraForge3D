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
		auto dt = m_UniformIds.find(name);
		if (dt != m_UniformIds.end()) return dt->second;
		auto location = glGetUniformLocation(m_Shader, name.c_str());
		m_UniformIds[name] = location;
		return location;
	}
	void SetUniform1i(const std::string& name, int value) { glUniform1i(GetUniformLocation(name), value); }
	void SetUniform1f(const std::string& name, float value) { glUniform1f(GetUniformLocation(name), value); }
	void SetUniform2f(const std::string& name, const glm::vec2& value) { glUniform2f(GetUniformLocation(name), value.x, value.y); }
	void SetUniform2f(const std::string& name, float value0, float value1) { glUniform2f(GetUniformLocation(name), value0, value1); }
	void SetUniform3f(const std::string& name, const glm::vec3& value) { glUniform3f(GetUniformLocation(name), value.x, value.y, value.z); }
	void SetUniform3f(const std::string& name, float value0, float value1, float value2) { glUniform3f(GetUniformLocation(name), value0, value1, value2); }
	void SetUniform4f(const std::string& name, const glm::vec4& value) { glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w); }
	void SetUniform4f(const std::string& name, float value0, float value1, float value2, float value3) { glUniform4f(GetUniformLocation(name), value0, value1, value2, value3); }
	void SetUniformMat4f(const std::string& name, const glm::mat4& matrix) { glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]); }
	inline int GetNativeShader() { return m_Shader; }
private:
	int m_Shader;
	std::unordered_map<std::string, uint32_t> m_UniformIds;
	int maxWorkGroupCount[3], maxWorkGroupSize[3], maxWorkGroupInvocations;
};