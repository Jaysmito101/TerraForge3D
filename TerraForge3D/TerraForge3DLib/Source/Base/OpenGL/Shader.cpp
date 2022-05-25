#include "Base/OpenGL/Shader.hpp"

#ifdef TF3D_OPENGL_BACKEND
namespace TerraForge3D
{

	namespace OpenGL
	{

		static GLuint CompileShader(GLenum shaderType, std::string& shaderSource, std::string name = "Shader")
		{
			GLuint shader = glCreateShader(shaderType);
			const GLchar* source = (const GLchar*)shaderSource.data();
			glShaderSource(shader, 1, &source, 0);
			glCompileShader(shader);
			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
				char* errorLog = new char[maxLength];
				memset(errorLog, 0, maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
				TF3D_LOG_ERROR("Error in compiling {0} Shader", name);
				TF3D_LOG_ERROR("Error Log: \n{0}", errorLog);
				delete[] errorLog;
				glDeleteShader(shader);
				return 0;
			}

			return shader;
		}		

		Shader::Shader()
		{
		}

		Shader::~Shader()
		{
			if (isCompiled || wasCompiled)
				Cleanup();
		}

		void Shader::Cleanup()
		{
			if (!wasCompiled) return;
			glDeleteProgram(handle);
			wasCompiled = false;
			isCompiled = false;
		}

		bool Shader::Compile()
		{
			TF3D_ASSERT(vertexSource.size() > 0, "Vertex shader source not set");
			TF3D_ASSERT(fragmentSource.size() > 0, "Fraagment shader source not set");
			if (isCompiled) return true;

			if (wasCompiled) Cleanup();
			
			GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource, "Vetex");
			GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource, "Fragment");
			if (vertexShader == 0 || fragmentShader == 0)
			{
				TF3D_LOG_ERROR("Failed to compile shader : {0}", name);
				return false;
			}

			handle = glCreateProgram();
			glAttachShader(handle, vertexShader);
			glAttachShader(handle, fragmentShader);
			glLinkProgram(handle);

			GLint isLinked = 0;
			glGetProgramiv(handle, GL_LINK_STATUS, &isLinked);
			if (isLinked == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &maxLength);
				char* errorLog = new char[maxLength];
				memset(errorLog, 0, maxLength);
				glGetProgramInfoLog(handle, maxLength, &maxLength, errorLog);
				glDeleteProgram(handle);
				glDeleteShader(vertexShader);
				glDeleteShader(fragmentShader);
				TF3D_LOG_ERROR("Error in linking shader : {0}", name);
				TF3D_LOG_ERROR("Error Log: \n{0}", errorLog);
				delete[] errorLog;
				return false;
			}
			
			glDetachShader(handle, vertexShader);
			glDetachShader(handle, fragmentShader);
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			isCompiled = true;
			wasCompiled = true;
		}

		bool Shader::LoadFromBinary(std::vector<uint32_t> binary)
		{
			TF3D_ASSERT(false, "Shader::LoadFromBinary not yet supported for OpenGL backend");
			return false;
		}

	}

}
#endif