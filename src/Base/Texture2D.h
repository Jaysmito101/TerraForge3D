#pragma once

#include <string>
#include <glad/glad.h>


	class Texture2D 
	{
	public:
		Texture2D(uint32_t width, uint32_t height);
		Texture2D(const std::string path, bool preserveData = true, bool readAlpha = false);
		virtual ~Texture2D();

		virtual uint32_t GetWidth() const  { return m_Width;  }
		virtual uint32_t GetHeight() const  { return m_Height; }
		virtual uint32_t GetRendererID() const { return m_RendererID; }
		virtual std::string GetPath() const  { return m_Path; }
		
		void Resize(int width, int height, bool resetOpenGL = true);

		virtual void SetData(void* data, uint32_t size, bool c = false);
		virtual void DeleteData();

		virtual void Bind(uint32_t slot = 0) const ;
		unsigned char* GetData();

		virtual bool IsLoaded() const { return m_IsLoaded; }
		
	private:
		unsigned char* m_Data = nullptr;
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
	};

