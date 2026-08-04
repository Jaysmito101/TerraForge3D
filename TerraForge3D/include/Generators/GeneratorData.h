#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <vector>

enum class GeneratorDataStorage
{
	R32F,
	R16F
};

class GeneratorData
{
public:
	GeneratorData();
	 explicit GeneratorData(GeneratorDataStorage storage);
	~GeneratorData();
	void Bind(uint32_t slot = 0);
	void BindAsTexture(uint32_t slot = 0);
	void Resize(size_t size);
	void SetData(const void* data, size_t offset = 0, size_t size = 0);
	bool CopyTo(const GeneratorData* other);
	float* GetCPUCopy();
	void GetData(void* data, size_t offset, size_t size);
	bool SaveToFile(const std::string& path);
	bool LoadFromFile(const std::string& path);
	inline size_t GetSize() const { return m_Size; }
	inline int32_t GetResolution() const { return m_Resolution; }
	inline GeneratorDataStorage GetStorage() const { return m_Storage; }
	static void SetDefaultStorage(GeneratorDataStorage storage);
	static GeneratorDataStorage GetDefaultStorage();
	static const char* GetDefaultStorageImageFormat();

private:
	uint32_t m_RendererID = 0;
	uint32_t m_Binding = 0;
	size_t m_Size = 0;
	int32_t m_Resolution = 0;
	GeneratorDataStorage m_Storage = GeneratorDataStorage::R32F;
	uint32_t m_InternalFormat = 0;
	uint32_t m_PixelType = 0;
	static GeneratorDataStorage s_DefaultStorage;
	static std::vector<GeneratorData*> s_Instances;
	void SetStorage(GeneratorDataStorage storage);
};
