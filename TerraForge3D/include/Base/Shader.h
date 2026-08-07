#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

namespace tf3d::base
{

    class ShaderCore
    {
    public:
        virtual ~ShaderCore() = default;

        inline void Bind()
        {
            glUseProgram(m_Shader);
        }

        inline void Unbind()
        {
            glUseProgram(0);
        }

        inline int GetNativeShader() const
        {
            return m_Shader;
        }

        inline void SetUniform1f(const std::string &name, float value)
        {
            glUniform1f(GetUniformLocation(name), value);
        }

        inline void SetUniform1fv(const std::string &name, const float *values, int count)
        {
            glUniform1fv(GetUniformLocation(name), count, values);
        }

        inline void SetUniform3f(const std::string &name, const float *value)
        {
            glUniform3f(GetUniformLocation(name), value[0], value[1], value[2]);
        }

        inline void SetUniform3f(const std::string &name, const glm::vec3 &value)
        {
            glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
        }

        inline void SetUniform3f(const std::string &name, float value0, float value1, float value2)
        {
            glUniform3f(GetUniformLocation(name), value0, value1, value2);
        }

        inline void SetUniform2f(const std::string &name, float value0, float value1)
        {
            glUniform2f(GetUniformLocation(name), value0, value1);
        }

        inline void SetUniform2f(const std::string &name, const glm::vec2 &value)
        {
            SetUniform2f(name, value.x, value.y);
        }

        inline void SetUniform2i(const std::string &name, int value0, int value1)
        {
            glUniform2i(GetUniformLocation(name), value0, value1);
        }

        inline void SetUniform4f(const std::string &name, const glm::vec4 &value)
        {
            glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
        }

        inline void SetUniform4f(const std::string &name, float value0, float value1, float value2, float value3)
        {
            glUniform4f(GetUniformLocation(name), value0, value1, value2, value3);
        }

        inline void SetUniform1i(const std::string &name, int value)
        {
            glUniform1i(GetUniformLocation(name), value);
        }

        inline void SetUniformMat4(const std::string &name, const glm::mat4 &value)
        {
            glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
        }

        inline int GetUniformLocation(const std::string &name)
        {
            const auto iterator = uniformLocations.find(name);
            if (iterator != uniformLocations.end()) {
                return iterator->second;
            }

            const int location = glGetUniformLocation(m_Shader, name.c_str());
            uniformLocations.emplace(name, location);
            return location;
        }

    protected:
        ShaderCore() = default;

        int m_Shader = 0;
        std::unordered_map<std::string, int> uniformLocations;
    };

    class GraphicsShader : public ShaderCore
    {
    public:
        GraphicsShader(std::string vertexSource, std::string fragmentSource, std::string geometrySource);
        GraphicsShader(std::string vertexSource, std::string fragmentSource);

        ~GraphicsShader() override;
    };

    class ComputeShader : public ShaderCore
    {
    public:
        ComputeShader(std::string source);
        ~ComputeShader() override;

        inline void Dispatch(int x, int y, int z)
        {
            glDispatchCompute(x, y, z);
        }

        inline void SetMemoryBarrier()
        {
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
        }

    private:
        int maxWorkGroupCount[3], maxWorkGroupSize[3], maxWorkGroupInvocations;
    };

} // namespace tf3d::base
