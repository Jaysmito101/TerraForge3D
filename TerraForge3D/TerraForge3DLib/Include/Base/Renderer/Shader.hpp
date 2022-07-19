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

		enum ShaderDataType
		{
			ShaderDataType_Mat3 = 0,
			ShaderDataType_Mat4,
			ShaderDataType_Vec2,
			ShaderDataType_Vec3,
			ShaderDataType_Vec4,
			ShaderDataType_IVec2,
			ShaderDataType_IVec3,
			ShaderDataType_IVec4,
			ShaderDataType_Bool
		};

		struct ShaderVar
		{
			std::string name = "";
			ShaderDataType type = ShaderDataType_Vec4;

			ShaderVar(std::string n, ShaderDataType t = ShaderDataType_Vec4)
				:name(n), type(t)
			{}
		};

		inline uint32_t GetShaderDataTypeSize(ShaderDataType type)
		{
			switch (type)
			{
				case ShaderDataType_Mat3	: return 3 * 3 * sizeof(float);
				case ShaderDataType_Mat4	: return 4 * 4 * sizeof(float);
				case ShaderDataType_Vec2	: return 2 * sizeof(float);
				case ShaderDataType_Vec3	: return 3 * sizeof(float);
				case ShaderDataType_Vec4	: return 4 * sizeof(float);
				case ShaderDataType_IVec2	: return 2 * sizeof(int);
				case ShaderDataType_IVec3	: return 3 * sizeof(int);
				case ShaderDataType_IVec4	: return 4 * sizeof(int);
				case ShaderDataType_Bool	: return sizeof(bool);
				default						: return 0;
			}
			return 0;
		}

		class Shader
		{
		public:

			Shader() = default;
			virtual ~Shader() = default;

			virtual void Cleanup() = 0;
			virtual bool Compile() = 0;
			virtual bool LoadFromBinary() = 0;

			inline bool IsCompiled() { return this->isCompiled; }
			inline std::vector<uint32_t> GetBinary(ShaderStage stage) 
			{
				if (isCompiled && loadedFromBinary)
				{
					switch (stage)
					{
					case ShaderStage_Vertex   : return binary.vertex;
					case ShaderStage_Geometry : return binary.geometry;
					case ShaderStage_Fragment : return binary.fragment;
						
					}
				}
				TF3D_ASSERT(false, "Shader binary not available");
				return std::vector<uint32_t>();
			}

			inline Shader* SetIncludeDir(std::string dir) { this->includeDir = dir; if (this->includeDir[this->includeDir.size() - 1] != PATH_SEPERATOR[0]) { this->includeDir += PATH_SEPERATOR; } return this; }
			inline Shader* SetCacheDir(std::string dir) { this->cacheDir = dir; return this; }
			inline Shader* SetMacro(std::string macroName, std::string value) { this->macros[macroName] = value; return this; }
			inline Shader* SetUBOLayout(std::vector<ShaderVar> layout) { this->uboLayout = layout; return this; };
			inline Shader* SetUniformsLayout(std::vector<ShaderVar> layout) { this->uniformsLayout = layout; return this; };

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


		public:
			bool isCompiled = false;
			bool wasCompiled = false;
			bool loadedFromBinary = false;
			std::string vertexSource = "";
			std::string fragmentSource = "";
			std::string includeDir = "";
			std::string cacheDir = ""; // For SPIRV cache
			std::unordered_map<std::string, std::string> macros;
			std::vector<ShaderVar> uboLayout;
			std::vector<ShaderVar> uniformsLayout;
			struct
			{
				std::vector<uint32_t> vertex;
				std::vector<uint32_t> geometry;
				std::vector<uint32_t> fragment;
			} binary;

		public:
			std::string name = "Shader";
		};

	}

}