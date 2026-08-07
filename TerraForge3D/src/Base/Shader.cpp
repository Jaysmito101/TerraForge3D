#include <Base.h>
#include <Shader.h>
#include <ShaderUtils.h>
#include <glm/gtc/type_ptr.hpp>

namespace tf3d::base
{

    GraphicsShader::GraphicsShader(std::string vertexSrc, std::string fragmentSrc, std::string geometrySource)
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

    GraphicsShader::GraphicsShader(std::string vertexSrc, std::string fragmentSrc)
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

    GraphicsShader::~GraphicsShader()
    {
        glDeleteProgram(m_Shader);
    }

    ComputeShader::ComputeShader(std::string source)
    {
        GLuint shader = CompileShader(source, GL_COMPUTE_SHADER, "Compute Shader");
        m_Shader      = CreateProgram();
        glAttachShader(m_Shader, shader);
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
            glDeleteShader(shader);
            TF3D_LOG_ERROR("Compute shader link failed: {}", errorLog);
            return;
        }

        glDetachShader(m_Shader, shader);
        glDeleteShader(shader);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroupCount[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGroupCount[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGroupCount[2]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
    }

    ComputeShader::~ComputeShader()
    {
        glDeleteProgram(m_Shader);
    }

} // namespace tf3d::base
