#include "Base/OpenGL/SharedStorageBuffer.hpp"

#ifdef TF3D_OPENGL_BACKEND

namespace TerraForge3D
{

	namespace OpenGL
	{
		
		SharedStorageBuffer::SharedStorageBuffer()
		{
		}

		SharedStorageBuffer::~SharedStorageBuffer()
		{
			if (autoDestory && isSetup)
				this->Destroy();				
		}

		bool SharedStorageBuffer::Setup()
		{
			TF3D_ASSERT(!isSetup, "Buffer is already setup");
			TF3D_ASSERT(size > 0, "Buffer size cannot be <= 0");
			
			glGenBuffers(1, &handle);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->size, NULL, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, this->binding, handle);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			isSetup = true;
			return true;
		}

		bool SharedStorageBuffer::Destroy()
		{
			TF3D_ASSERT(isSetup, "Buffer is not yet setup");
			
			glDeleteBuffers(1, &handle);
			isSetup = false;
			return true;
		}

		bool SharedStorageBuffer::SetData(void* data, uint64_t lSize, uint64_t lOffset)
		{
			TF3D_ASSERT(isSetup, "Buffer is not yet setup");
			TF3D_ASSERT(lSize <= this->size, "Invalid size");
			TF3D_ASSERT(lOffset>= 0, "Invalid offset");
			TF3D_ASSERT((lOffset + lSize) <= this->size, "Invalid offset");
			
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->handle);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, lOffset, lSize, data);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, this->binding, this->handle);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			return true;
		}

		bool SharedStorageBuffer::GetData(void* data, uint64_t lSize, uint64_t lOffset)
		{
			TF3D_ASSERT(isSetup, "Buffer is not yet setup");
			TF3D_ASSERT(lSize <= this->size, "Invalid size");
			TF3D_ASSERT(lOffset>= 0, "Invalid offset");
			TF3D_ASSERT((lOffset + lSize) <= this->size, "Invalid offset");

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->handle); 
			glGetNamedBufferSubData(this->handle, lOffset, lSize, data);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			return true;
		}


	}

}

#endif