#include <Base.h>
#include <Shader.h>
#include <ShaderUtils.h>
#include <glm/gtc/type_ptr.hpp>

namespace tf3d::base
{

    Shader::Shader(std::string vertexSrc, std::string fragmentSrc, std::string geometrySource)
    {
        GLuint vertShader = CompileShader(vertexSrc, GL_VERTEX_SHADER, "Vertex");
        GLuint geomShader = CompileShader(geometrySource, GL_GEOMETRY_SHADER, "Geometry");
        GLuint fragShader = CompileShader(fragmentSrc, GL_FRAGMENT_SHADER, "Fragment");
        m_Shader          = CreateProgram();
        glAttachShader(m_Shader, vertShader);
        glAttachShader(m_Shader, geomShader);
        glAttachShader(m_Shader, fragShader);
        glLinkProgram(m_Shader);
        GLint isLinked = 0;
        glGetProgramiv(m_Shader, GL_LINK_STATUS, (int *)&isLinked);

        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
            char *errorLog = (char *)malloc(maxLength);
            memset(errorLog, 0, maxLength);
            glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
            glDeleteProgram(m_Shader);
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            TF3D_LOG_ERROR("Shader link failed: {}", errorLog);
            return;
        }

        glDetachShader(m_Shader, vertShader);
        glDetachShader(m_Shader, fragShader);
        glDetachShader(m_Shader, geomShader);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteShader(geomShader);
    }

    Shader::Shader(std::string vertexSrc, std::string fragmentSrc)
    {
        GLuint vertShader = CompileShader(vertexSrc, GL_VERTEX_SHADER, "Vertex");
        GLuint fragShader = CompileShader(fragmentSrc, GL_FRAGMENT_SHADER, "Fragment");
        m_Shader          = CreateProgram();
        glAttachShader(m_Shader, vertShader);
        glAttachShader(m_Shader, fragShader);
        glLinkProgram(m_Shader);
        GLint isLinked = 0;
        glGetProgramiv(m_Shader, GL_LINK_STATUS, (int *)&isLinked);

        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
            char *errorLog = (char *)malloc(maxLength);
            memset(errorLog, 0, maxLength);
            glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
            glDeleteProgram(m_Shader);
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            TF3D_LOG_ERROR("Shader link failed: {}", errorLog);
            return;
        }

        glDetachShader(m_Shader, vertShader);
        glDetachShader(m_Shader, fragShader);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
    }

    void Shader::Bind()
    {
        glUseProgram(m_Shader);
    }

    void Shader::SetLightPos(glm::vec3 &pos)
    {
        if (m_LightPosUniformID <= 0) {
            m_LightPosUniformID = glGetUniformLocation(m_Shader, "_LightPosition");
        }

        glUniform3fv(m_LightPosUniformID, 1, glm::value_ptr(pos));
    }

    void Shader::SetLightCol(float *col)
    {
        if (m_LightColUniformID <= 0) {
            m_LightColUniformID = glGetUniformLocation(m_Shader, "_LightColor");
        }

        glUniform3fv(m_LightColUniformID, 1, col);
    }

    void Shader::SetTime(float *time)
    {
        if (m_TimeUniformID <= 0) {
            m_TimeUniformID = glGetUniformLocation(m_Shader, "_Time");
        }

        glUniform1fv(m_TimeUniformID, 1, time);
    }

    void Shader::SetMPV(const glm::mat4 &pv)
    {
        if (m_UniformId <= 0) {
            m_UniformId = glGetUniformLocation(m_Shader, "_PV");
        }

        glUniformMatrix4fv(m_UniformId, 1, GL_FALSE, glm::value_ptr(pv));
    }

    int Shader::GetUniformLocation(const std::string &name)
    {
        auto it = uniformLocations.find(name);
        if (it != uniformLocations.end())
            return it->second;

        const int location = glGetUniformLocation(m_Shader, name.c_str());
        uniformLocations.emplace(name, location);
        return location;
    }

    void Shader::SetUniformf(const std::string &name, float value)
    {
        SetUniform1f(name, value);
    }

    void Shader::SetUniform1f(const std::string &name, float value)
    {
        glUniform1f(GetUniformLocation(name), value);
    }

    void Shader::SetUniform3f(const std::string &name, const float *value)
    {
        glUniform3f(GetUniformLocation(name), value[0], value[1], value[2]);
    }

    void Shader::SetUniform3f(const std::string &name, const glm::vec3 &value)
    {
        glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
    }

    void Shader::SetUniform2f(const std::string &name, float value0, float value1)
    {
        glUniform2f(GetUniformLocation(name), value0, value1);
    }

    void Shader::SetUniform2f(const std::string &name, const glm::vec2 &value)
    {
        SetUniform2f(name, value.x, value.y);
    }

    void Shader::SetUniformi(const std::string &name, int value)
    {
        SetUniform1i(name, value);
    }

    void Shader::SetUniform1i(const std::string &name, int value)
    {
        glUniform1i(GetUniformLocation(name), value);
    }

    void Shader::SetUniformMat4(const std::string &name, const glm::mat4 &value)
    {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::Unbind()
    {
        glUseProgram(0);
    }

    Shader::~Shader()
    {
        glDeleteProgram(m_Shader);
    }

} // namespace tf3d::base
