#include "Base/OpenGL/Shader.hpp"

#include <shaderc/shaderc.hpp>
#include <array>

#ifdef TF3D_OPENGL_BACKEND
namespace TerraForge3D
{

	namespace OpenGL
	{
		class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
		{
		public:
			ShaderIncluder(std::string& includeDir)
			{
				this->includeDir = includeDir;
				if (includeDir[includeDir.size() - 1] != PATH_SEPERATOR[0])
					includeDir += PATH_SEPERATOR;
			}

		private:
			shaderc_include_result* GetInclude(
				const char* requestedSource,
				shaderc_include_type type,
				const char* requestingSource,
				size_t includeDepth
				)
			{
				auto result = new shaderc_include_result;
				std::string fileName = std::string(requestedSource);
				fileName = includeDir + fileName;
				std::ifstream ifs;
				ifs.open(fileName);
				if (!ifs.good())
				{
					TF3D_LOG_WARN("Shader file {0} could not be included [File not found]", fileName);
					result->content = "";
					result->content_length = 0;
					result->source_name = "";
					result->source_name_length = 0;
					result->user_data = nullptr;
					return result;
				}
				std::stringstream ss;
				ss << ifs.rdbuf();
				ifs.close();
				auto container = new std::array<std::string, 2>;
				(*container)[0] = fileName;
				(*container)[1] = ss.str();
				result->user_data = container;
				result->source_name = (*container)[0].data();
				result->source_name_length = (*container)[0].size();
				result->content = (*container)[1].data();
				result->content_length = (*container)[1].size();
				// TF3D_LOG_TRACE("Shader Include\nFile: {0}\nResult: {1}", fileName.data(), result->content);
				return result;
			}

			void ReleaseInclude(shaderc_include_result* result) override
			{
				delete reinterpret_cast<std::array<std::string, 2>*>(result->user_data);
				delete result;
			}

		private:
			std::string includeDir = "";
		};


		static std::string PreprocessShader(std::string& source, shaderc_shader_kind kind, std::unordered_map<std::string, std::string>& macros, std::string& includeDir, std::string sourceName = "shader.glsl")
		{
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;

			for (auto& [name, value] : macros)
				options.AddMacroDefinition(name, value);

			options.AddMacroDefinition("TF3D_OPENGL", "");

			options.SetIncluder(std::make_unique<ShaderIncluder>(includeDir));

#ifdef TF3D_DEBUG
//			options.SetGenerateDebugInfo();
#endif

			shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(source, kind, sourceName.data(), options);
			
			std::string resultSource = "";

			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				TF3D_LOG_ERROR("Shader preprocessing failed\n{0}", result.GetErrorMessage());
			}
			else
				resultSource = std::string(result.begin(), result.end());

			std::istringstream f(resultSource);
			std::stringstream ss;
			std::string line;
			while (std::getline(f, line)) {
				if (line[0] == '#' && line.find("#version") == std::string::npos)
					continue;
				ss << line << "\n";
			}

			return ss.str();
		}

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

			vertexSource = PreprocessShader(vertexSource, shaderc_glsl_vertex_shader, macros, includeDir, "VertexShader.glsl");
			fragmentSource = PreprocessShader(fragmentSource, shaderc_glsl_fragment_shader, macros, includeDir, "FragmentShader.glsl");
		

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

			return true;
		}

		bool Shader::LoadFromBinary(std::vector<uint32_t> binary)
		{
			TF3D_ASSERT(false, "Shader::LoadFromBinary not yet supported for OpenGL backend");
			return false;
		}

	}

}
#endif