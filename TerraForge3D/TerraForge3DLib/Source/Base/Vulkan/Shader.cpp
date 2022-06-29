#include "Base/Vulkan/Shader.hpp"
#include "Base/Vulkan/GraphicsDevice.hpp"

#include <shaderc/shaderc.hpp>
#include <array>

#ifdef TF3D_VULKAN_BACKEND
namespace TerraForge3D
{

	namespace Vulkan
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

		static std::vector<char> PreprocessAndCompileShader(std::string& source, shaderc_shader_kind kind, std::unordered_map<std::string, std::string>& macros, std::string& includeDir, std::string sourceName = "shader.glsl")
		{
			shaderc::Compiler compiler;
			shaderc::CompileOptions options;

			for (auto& [name, value] : macros)
				options.AddMacroDefinition(name, value);

			options.AddMacroDefinition("TF3D_VULKAN", "");

			options.SetIncluder(std::make_unique<ShaderIncluder>(includeDir));

#ifdef TF3D_DEBUG
			options.AddMacroDefinition("TF3D_DEBUG", "");
			//			options.SetGenerateDebugInfo();
#else
			options.AddMacroDefinition("TF3D_RELEASE", "");
#endif

			shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(source, kind, sourceName.data(), options);

			std::string resultSource = "";

			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				TF3D_LOG_ERROR("Shader preprocessing failed\n{0}", result.GetErrorMessage());
			}
			else
				resultSource = std::string(result.begin(), result.end());

			/*
			std::istringstream f(resultSource);
			std::stringstream ss;
			std::string line;
			while (std::getline(f, line)) {
				if (line[0] == '#' && line.find("#version") == std::string::npos)
					continue;
				ss << line << "\n";
			}

			return ss.str();
			*/

			shaderc::SpvCompilationResult resultSpv = compiler.CompileGlslToSpv(resultSource, kind, sourceName.data());
			
			return std::vector<char>(resultSpv.begin(), resultSpv.end());
		}

		static VkShaderModule CreateShaderModule(std::vector<char>& binary, VkDevice device)
		{
			VkShaderModuleCreateInfo shaderModuleCreateInfo{};
			shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCreateInfo.codeSize = binary.size();
			shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());
			
			VkShaderModule shaderModule;
			TF3D_VK_CALL(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule));
			return shaderModule;
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
			
			TF3D_ASSERT(device, "Device handle is NULL");

			vkDeviceWaitIdle(device->handle);
			vkDestroyShaderModule(device->handle, fragmentShaderModule, nullptr);
			vkDestroyShaderModule(device->handle, vertexShaderModule, nullptr);

			isCompiled = false;
			wasCompiled = false;
		}

		bool Shader::Compile()
		{
			TF3D_ASSERT(vertexSource.size() > 0, "Vertex shader source not set");
			TF3D_ASSERT(fragmentSource.size() > 0, "Fraagment shader source not set");
			if (isCompiled) return true;

			if (wasCompiled) Cleanup();

			if (!device)
				device = GraphicsDevice::Get();


			binary.vertex = PreprocessAndCompileShader(vertexSource, shaderc_glsl_vertex_shader, macros, includeDir, "VertexShader.glsl");
			binary.fragment = PreprocessAndCompileShader(fragmentSource, shaderc_glsl_fragment_shader, macros, includeDir, "FragmentShader.glsl");

			vertexShaderModule = CreateShaderModule(binary.vertex, device->handle);
			fragmentShaderModule = CreateShaderModule(binary.fragment, device->handle);

			isCompiled = true;
			wasCompiled = true;

			LoadPushConstantLocations();
			
			return true;
		}

		bool Shader::LoadFromBinary()
		{
			if (isCompiled) return true;

			if (wasCompiled) Cleanup();

			if (!device)
				device = GraphicsDevice::Get();
			
			vertexShaderModule = CreateShaderModule(binary.vertex, device->handle);
			fragmentShaderModule = CreateShaderModule(binary.fragment, device->handle);

			isCompiled = true;
			wasCompiled = true;

			LoadPushConstantLocations();

			return true;
		}

		void Shader::LoadPushConstantLocations()
		{
			pushConstantsLocations.clear();
			uint32_t offset = 0;
			for (int i = 0; i < uniformsLayout.size(); i++)
			{
				auto& unifrom = uniformsLayout[i];
				uint32_t size = RendererAPI::GetShaderDataTypeSize(unifrom.type);
				pushConstantsLocations[unifrom.name] = { size, offset };
				offset += size;
			}
			pushConstantsSize = offset;
		}




	}

}
#endif