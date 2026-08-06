#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace tf3d::base
{

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
        void SetMPV(const glm::mat4 &);

        void SetUniformf(const std::string &name, float value);
        void SetUniform1f(const std::string &name, float value);
        void SetUniform3f(const std::string &name, const float *value);
        void SetUniform3f(const std::string &name, const glm::vec3 &value);
        void SetUniform3f(const std::string &name, float value0, float value1, float value2);
        void SetUniform2f(const std::string &name, float value0, float value1);
        void SetUniform2f(const std::string &name, const glm::vec2 &value);
        void SetUniform2i(const std::string &name, int value0, int value1);
        void SetUniform4f(const std::string &name, float value0, float value1, float value2, float value3);
        void SetUniformi(const std::string &name, int value);
        void SetUniform1i(const std::string &name, int value);
        void SetUniformMat4(const std::string &name, const glm::mat4 &value);

        void Unbind();

        inline int GetNativeShader()
        {
            return m_Shader;
        }

        int m_Shader = 0, m_UniformId = 0, m_LightPosUniformID = 0, m_LightColUniformID = 0, m_TimeUniformID = 0;
        std::unordered_map<std::string, int> uniformLocations;

    private:
        int GetUniformLocation(const std::string &name);
    };

} // namespace tf3d::base
using tf3d::base::Shader;
