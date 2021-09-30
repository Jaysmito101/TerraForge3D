#include <Base.h>
#include <Shader.h>
#include <windows.h>
#include <glm/gtc/type_ptr.hpp>


static void Log(const char* log) {
	std::cout << log << std::endl;
};

static void Log(std::string log) {
	std::cout << log << std::endl;
};

static int CompileShader(std::string shaderSrc, GLenum shaderType, std::string name) {
	GLuint shader = glCreateShader(shaderType);
	const GLchar* source = (const GLchar*)shaderSrc.c_str();
	glShaderSource(shader, 1, &source, 0);
	glCompileShader(shader);
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		char* errorLog = (char*)malloc(maxLength);
		memset(errorLog, 0, maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
		Log(std::string("Error in Compiling ") + name + " Shader : ");
		Log(errorLog);
		glDeleteShader(shader);
	}
	return shader;
}

static int CreateProgram() {
	GLuint program = glCreateProgram();
	return program;
}

Shader::Shader(std::string vertexSrc, std::string fragmentSrc, std::string geometrySource)
{
	GLuint vertShader = CompileShader(vertexSrc, GL_VERTEX_SHADER, "Vertex");
	GLuint geomShader = CompileShader(geometrySource, GL_GEOMETRY_SHADER, "Geometry");
	GLuint fragShader = CompileShader(fragmentSrc, GL_FRAGMENT_SHADER, "Fragment");
	m_Shader = CreateProgram();
	glAttachShader(m_Shader, vertShader);
	glAttachShader(m_Shader, geomShader);
	glAttachShader(m_Shader, fragShader);
	glLinkProgram(m_Shader);
	GLint isLinked = 0;
	glGetProgramiv(m_Shader, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
		char* errorLog = (char*)malloc(maxLength);
		memset(errorLog, 0, maxLength);
		glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
		glDeleteProgram(m_Shader);
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		Log(errorLog);
	}
	glDetachShader(m_Shader, vertShader);
	glDetachShader(m_Shader, fragShader);
	glDetachShader(m_Shader, geomShader);
}


Shader::Shader(std::string vertexSrc, std::string fragmentSrc)
{
	GLuint vertShader = CompileShader(vertexSrc, GL_VERTEX_SHADER, "Vertex");
	GLuint fragShader = CompileShader(fragmentSrc, GL_FRAGMENT_SHADER, "Fragment");
	m_Shader = CreateProgram();
	glAttachShader(m_Shader, vertShader);
	glAttachShader(m_Shader, fragShader);
	glLinkProgram(m_Shader);
	GLint isLinked = 0;
	glGetProgramiv(m_Shader, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);
		char* errorLog = (char*)malloc(maxLength);
		memset(errorLog, 0, maxLength);
		glGetProgramInfoLog(m_Shader, maxLength, &maxLength, errorLog);
		glDeleteProgram(m_Shader);
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		Log(errorLog);
	}
	glDetachShader(m_Shader, vertShader);
	glDetachShader(m_Shader, fragShader);
}


void Shader::Bind()
{
	glUseProgram(m_Shader);
}

void Shader::SetLightPos(glm::vec3& pos)
{
	if (m_LightPosUniformID <= 0) {
		m_LightPosUniformID = glGetUniformLocation(m_Shader, "_LightPosition");
	}
	glUniform3fv(m_LightPosUniformID, 1, glm::value_ptr(pos));
}

void Shader::SetLightCol(float* col)
{
	if (m_LightColUniformID <= 0) {
		m_LightColUniformID = glGetUniformLocation(m_Shader, "_LightColor");
	}
	glUniform3fv(m_LightColUniformID, 1, col);
}

void Shader::SetTime(float* time)
{
	if (m_TimeUniformID <= 0) {
		m_TimeUniformID = glGetUniformLocation(m_Shader, "_Time");
	}
	glUniform1fv(m_TimeUniformID, 1, time);
}

void Shader::SetMPV(glm::mat4& pv)
{
	if (m_UniformId <= 0) {
		m_UniformId = glGetUniformLocation(m_Shader, "_PV");
	}
	glUniformMatrix4fv(m_UniformId, 1, GL_FALSE, glm::value_ptr(pv));
}

void Shader::SetUniformf(std::string name, float value)
{
	if (uniformLocations.find(name) == uniformLocations.end()) {
		uint32_t loc = glGetUniformLocation(m_Shader, name.c_str());
		uniformLocations.insert(std::make_pair(name, loc));
		glUniform1f(loc, value);
	}
	else {
		uint32_t loc = uniformLocations[name];
		glUniform1f(loc, value);
	}
}

void Shader::SetUniform3f(std::string name, float* value)
{
	if (uniformLocations.find(name) == uniformLocations.end()) {
		uint32_t loc = glGetUniformLocation(m_Shader, name.c_str());
		uniformLocations.insert(std::make_pair(name, loc));
		glUniform3fv(loc, 1, value);
	}
	else {
		uint32_t loc = uniformLocations[name];
		glUniform3fv(loc, 1, value);
	}
}

void Shader::SetUniformi(std::string name, int value)
{
	if (uniformLocations.find(name) == uniformLocations.end()) {
		uint32_t loc = glGetUniformLocation(m_Shader, name.c_str());
		uniformLocations.insert(std::make_pair(name, loc));
		glUniform1i(loc, value);
	}
	else {
		uint32_t loc = uniformLocations[name];
		glUniform1i(loc, value);
	}
}


void Shader::SetUniformMAt4(std::string name, glm::mat4& value)
{
	if (uniformLocations.find(name) == uniformLocations.end()) {
		uint32_t loc = glGetUniformLocation(m_Shader, name.c_str());
		uniformLocations.insert(std::make_pair(name, loc));
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
	}
	else {
		uint32_t loc = uniformLocations[name];
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
	}
}

void Shader::Unbind()
{
	glUseProgram(0);
}

Shader::~Shader() 
{
	glDeleteProgram(m_Shader);
}
