#pragma once

#include <vector>
#include <string>

class Heightmap
{
public:
	Heightmap(uint32_t width, uint32_t height) : m_Width(width), m_Height(height) {}
	Heightmap(const std::string path);
	~Heightmap();

	std::string GetPath() const
	{
		return m_Path;
	}
	uint32_t GetWidth() const
	{
		return m_Width;
	}
	uint32_t GetHeight() const
	{
		return m_Height;
	}
	uint16_t *GetData() const
	{
		return m_Data;
	}
	uint32_t GetRendererID() const
	{
		return m_RendererID;
	}

	uint16_t Sample(float x, float y, bool interpolated) const;

private:
	uint32_t m_Width, m_Height;
	uint16_t *m_Data;
	uint32_t m_RendererID;

	std::string m_Path;

	uint16_t Get(uint32_t x, uint32_t y) const;
};