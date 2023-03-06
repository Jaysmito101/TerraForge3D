#include "ComputeShader.h"
#include <ShaderUtils.h>


ComputeShader::ComputeShader(std::string source)
{
	GLuint shader = CompileShader(source, GL_COMPUTE_SHADER, "Compute Shader");
	m_Shader = CreateProgram();
	glAttachShader(m_Shader, shader);
	glLinkProgram(m_Shader);
	GLint isLinked = 0;
	glGetProgramiv(m_Shader, GL_LINK_STATUS, (int *)&isLinked);

	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
		char *errorLog = (char *)malloc(maxLength);
		memset(errorLog, 0, maxLength);
		glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
		glDeleteProgram(m_Shader);
		glDeleteShader(shader);
		Log(errorLog);
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

