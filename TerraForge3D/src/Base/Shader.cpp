#include <Base.h>
#include <Base/Logging/Logger.h>
#include <Shader.h>
#include <glm/gtc/type_ptr.hpp>

namespace tf3d::base
{

    namespace
    {
        GLuint CompileShader(const std::string &shaderSrc, GLenum shaderType, const std::string &name)
        {
            GLuint shader        = glCreateShader(shaderType);
            const GLchar *source = shaderSrc.c_str();
            glShaderSource(shader, 1, &source, 0);
            glCompileShader(shader);
            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

            if (isCompiled == GL_FALSE) {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
                char *errorLog = (char *)malloc(maxLength);
                memset(errorLog, 0, maxLength);
                glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
                TF3D_LOG_ERROR("Failed to compile {} shader", name);
                TF3D_LOG_DEBUG("Shader source:\n{}", shaderSrc);
                TF3D_LOG_ERROR("Shader compiler output: {}", errorLog);
                glDeleteShader(shader);
                free(errorLog);
                return 0;
            }

            return shader;
        }

        GLuint CreateProgram()
        {
            return glCreateProgram();
        }
    } // namespace

    GraphicsShader::GraphicsShader(std::string vertexSrc, std::string fragmentSrc, std::string geometrySource)
    {
        GLuint vertShader = CompileShader(vertexSrc, GL_VERTEX_SHADER, "Vertex");
        GLuint geomShader = CompileShader(geometrySource, GL_GEOMETRY_SHADER, "Geometry");
        GLuint fragShader = CompileShader(fragmentSrc, GL_FRAGMENT_SHADER, "Fragment");

        if (vertShader == 0 || geomShader == 0 || fragShader == 0) {
            if (vertShader != 0)
                glDeleteShader(vertShader);
            if (geomShader != 0)
                glDeleteShader(geomShader);
            if (fragShader != 0)
                glDeleteShader(fragShader);
            return;
        }

        m_Shader = CreateProgram();
        glAttachShader(m_Shader, vertShader);
        glAttachShader(m_Shader, geomShader);
        glAttachShader(m_Shader, fragShader);
        glLinkProgram(m_Shader);
        GLint isLinked = 0;
        glGetProgramiv(m_Shader, GL_LINK_STATUS, &isLinked);

        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
            char *errorLog = (char *)malloc(maxLength);
            memset(errorLog, 0, maxLength);
            glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
            glDeleteProgram(m_Shader);
            m_Shader = 0;
            glDeleteShader(vertShader);
            glDeleteShader(geomShader);
            glDeleteShader(fragShader);
            TF3D_LOG_ERROR("Shader link failed: {}", errorLog);
            free(errorLog);
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

        if (vertShader == 0 || fragShader == 0) {
            if (vertShader != 0)
                glDeleteShader(vertShader);
            if (fragShader != 0)
                glDeleteShader(fragShader);
            return;
        }

        m_Shader = CreateProgram();
        glAttachShader(m_Shader, vertShader);
        glAttachShader(m_Shader, fragShader);
        glLinkProgram(m_Shader);
        GLint isLinked = 0;
        glGetProgramiv(m_Shader, GL_LINK_STATUS, &isLinked);

        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
            char *errorLog = (char *)malloc(maxLength);
            memset(errorLog, 0, maxLength);
            glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
            glDeleteProgram(m_Shader);
            m_Shader = 0;
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            TF3D_LOG_ERROR("Shader link failed: {}", errorLog);
            free(errorLog);
            return;
        }

        glDetachShader(m_Shader, vertShader);
        glDetachShader(m_Shader, fragShader);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
    }

    ComputeShader::ComputeShader(std::string source)
    {
        GLuint shader = CompileShader(source, GL_COMPUTE_SHADER, "Compute Shader");

        if (shader == 0)
            return;

        m_Shader = CreateProgram();
        glAttachShader(m_Shader, shader);
        glLinkProgram(m_Shader);
        GLint isLinked = 0;
        glGetProgramiv(m_Shader, GL_LINK_STATUS, &isLinked);

        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
            char *errorLog = (char *)malloc(maxLength);
            memset(errorLog, 0, maxLength);
            glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
            glDeleteProgram(m_Shader);
            m_Shader = 0;
            glDeleteShader(shader);
            TF3D_LOG_ERROR("Compute shader link failed: {}", errorLog);
            free(errorLog);
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

} // namespace tf3d::base
