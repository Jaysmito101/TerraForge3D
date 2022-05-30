#pragma once
#include "Base/Core/Core.hpp"

#include <glm/gtc/type_ptr.hpp>


namespace TerraForge3D
{

	namespace RendererAPI
	{
		enum ShaderStage
		{
			ShaderStage_Vertex = 0,
			ShaderStage_Geometry,
			ShaderStage_Fragment
		};

		class Shader
		{
		public:

			Shader() = default;
			~Shader() = default;

			virtual void Cleanup() = 0;
			virtual bool Compile() = 0;
			virtual bool LoadFromBinary(std::vector<uint32_t> binary) = 0;

			inline bool IsCompiled() { return this->isCompiled; }
			inline std::vector<uint32_t> GetBinary() { if (isCompiled && loadedFromBinary) return binary; TF3D_ASSERT(false, "Shader binary not available"); }

			inline Shader* SetSource(std::string& source, ShaderStage stage)
			{
				switch (stage)
				{
				case ShaderStage_Vertex:
					this->vertexSource = source;
					break;
				case ShaderStage_Geometry:
					TF3D_ASSERT(false, "Geometry shaders are not yet suppoted");
					break;
				case ShaderStage_Fragment:
					this->fragmentSource = source;
					break;
				default:
					TF3D_ASSERT(false, "Unknown shader stage!");
				}
				isCompiled = false;
				return this;
			}


		public:
			static Shader* Create();


		protected:
			bool isCompiled = false;
			bool wasCompiled = false;
			bool loadedFromBinary = false;
			std::string vertexSource = "";
			std::string fragmentSource = "";
			std::vector<uint32_t> binary;

		public:
			std::string name = "Shader";
		};

	}

}