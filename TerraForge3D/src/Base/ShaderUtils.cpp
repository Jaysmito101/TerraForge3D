#include <ShaderUtils.h>

#include <iostream>


#include <Utils.h>

int CompileShader(std::string shaderSrc, GLenum shaderType, std::string name)
{
	GLuint shader = glCreateShader(shaderType);
	const GLchar *source = (const GLchar *)shaderSrc.c_str();
	glShaderSource(shader, 1, &source, 0);
	glCompileShader(shader);
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		char *errorLog = (char *)malloc(maxLength);
		memset(errorLog, 0, maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
		Log(std::string("Error in Compiling ") + name + " Shader : ");
		Log("Shader Source : \n" + shaderSrc + "\n");
		Log(errorLog);
		glDeleteShader(shader);
	}

	return shader;
}

int CreateProgram()
{
	GLuint program = glCreateProgram();
	return program;
}
