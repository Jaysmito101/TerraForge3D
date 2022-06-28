#pragma once
#include "Base/OpenGL/Core.hpp"
#include "Base/Renderer/Shader.hpp"

#ifdef TF3D_OPENGL_BACKEND
namespace TerraForge3D
{

	namespace OpenGL
	{

		class Shader : public RendererAPI::Shader
		{
		public:
			Shader();
			~Shader();
			
			virtual void Cleanup() override;
			virtual bool Compile() override;
			virtual bool LoadFromBinary() override;

			void LoadUniformLocations();

			inline bool UniformExists(const std::string& name) { return uniformLocations.find(name) != uniformLocations.end(); }
			inline std::pair<GLuint, RendererAPI::ShaderDataType>& GetUniform(const std::string& name) { return uniformLocations[name]; }
				
		public:
			GLuint handle = 0;
			std::unordered_map<std::string, std::pair<GLuint, RendererAPI::ShaderDataType>> uniformLocations;
		};

	}

}
#endif