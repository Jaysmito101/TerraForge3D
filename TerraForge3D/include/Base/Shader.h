#pragma once

#include "Profiler.h"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

namespace tf3d::base
{

    class ShaderCore
    {
    public:
        ShaderCore(const ShaderCore &)            = delete;
        ShaderCore &operator=(const ShaderCore &) = delete;

        ShaderCore(ShaderCore &&other) noexcept
            : m_Shader(std::exchange(other.m_Shader, 0)), uniformLocations(std::move(other.uniformLocations))
        {
        }

        ShaderCore &operator=(ShaderCore &&other) noexcept
        {
            if (this == &other)
                return *this;

            Release();
            m_Shader         = std::exchange(other.m_Shader, 0);
            uniformLocations = std::move(other.uniformLocations);
            return *this;
        }

        virtual ~ShaderCore()
        {
            Release();
        }

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

        inline bool IsValid() const
        {
            return m_Shader != 0;
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

        void Release() noexcept
        {
            if (m_Shader != 0) {
                glDeleteProgram(m_Shader);
                m_Shader = 0;
            }
            uniformLocations.clear();
        }

        int m_Shader = 0;
        std::unordered_map<std::string, int> uniformLocations;
    };

    class GraphicsShader : public ShaderCore
    {
    public:
        GraphicsShader() = default;
        GraphicsShader(std::string vertexSource, std::string fragmentSource, std::string geometrySource);
        GraphicsShader(std::string vertexSource, std::string fragmentSource);

        GraphicsShader(const GraphicsShader &)                = delete;
        GraphicsShader &operator=(const GraphicsShader &)     = delete;
        GraphicsShader(GraphicsShader &&) noexcept            = default;
        GraphicsShader &operator=(GraphicsShader &&) noexcept = default;
        ~GraphicsShader() override                            = default;
    };

    class ComputeShader : public ShaderCore
    {
    public:
        ComputeShader() = default;
        ComputeShader(std::string source);

        ComputeShader(const ComputeShader &)                = delete;
        ComputeShader &operator=(const ComputeShader &)     = delete;
        ComputeShader(ComputeShader &&) noexcept            = default;
        ComputeShader &operator=(ComputeShader &&) noexcept = default;
        ~ComputeShader() override                           = default;

        inline void Dispatch(int x, int y, int z)
        {
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/dispatches", 1.0, PerformanceMonitor::Domain::Gpu);
            TF3D_PROFILE_VALUE_DOMAIN("gpu/dispatch/workgroups", static_cast<uint64_t>(std::max(x, 0)),
                                      static_cast<uint64_t>(std::max(y, 0)), static_cast<uint64_t>(std::max(z, 0)),
                                      PerformanceMonitor::Domain::Gpu);
            glDispatchCompute(x, y, z);
        }

        inline void SetMemoryBarrier()
        {
            TF3D_PROFILE_COUNTER_DOMAIN("gpu/barriers", 1.0, PerformanceMonitor::Domain::Gpu);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
        }

    private:
        int maxWorkGroupCount[3]{};
        int maxWorkGroupSize[3]{};
        int maxWorkGroupInvocations = 0;
    };

} // namespace tf3d::base
