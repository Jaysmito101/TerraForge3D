#include <ShaderUtils.h>

#include <iostream>

#include "Utils/Utils.h"

namespace tf3d::base
{

    int CompileShader(std::string shaderSrc, GLenum shaderType, std::string name)
    {
        GLuint shader        = glCreateShader(shaderType);
        const GLchar *source = (const GLchar *)shaderSrc.c_str();
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

    int CreateProgram()
    {
        GLuint program = glCreateProgram();
        return program;
    }

} // namespace tf3d::base
