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
			virtual bool LoadFromBinary(std::vector<uint32_t> binary) override;
				
		public:
			GLuint handle = 0;
		};

	}

}
#endif