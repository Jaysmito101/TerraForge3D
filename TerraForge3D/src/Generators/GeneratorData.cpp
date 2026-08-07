#include "Generators/GeneratorData.h"
#include "Base/Base.h"
#include "Profiler.h"

#include <algorithm>
#include <cmath>

namespace tf3d::generators
{

    GeneratorDataStorage GeneratorData::s_DefaultStorage = GeneratorDataStorage::R32F;
    std::vector<GeneratorData *> GeneratorData::s_Instances;

    static const char *s_GeneratorDataSaveFileHeader = "TF3D.3.0.GENDATA";

    GeneratorData::GeneratorData()
        : GeneratorData(s_DefaultStorage)
    {
    }

    GeneratorData::GeneratorData(GeneratorDataStorage storage)
        : m_Storage(storage)
    {
        m_InternalFormat = m_Storage == GeneratorDataStorage::R16F ? GL_R16F : GL_R32F;
        m_PixelType      = m_Storage == GeneratorDataStorage::R16F ? GL_HALF_FLOAT : GL_FLOAT;
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        s_Instances.push_back(this);
    }

    GeneratorData::~GeneratorData()
    {
        glDeleteTextures(1, &m_RendererID);
        s_Instances.erase(std::remove(s_Instances.begin(), s_Instances.end(), this), s_Instances.end());
    }

    void GeneratorData::Bind(uint32_t slot)
    {
        m_Binding = slot;
        glBindImageTexture(slot, m_RendererID, 0, GL_FALSE, 0, GL_READ_WRITE, m_InternalFormat);
    }

    void GeneratorData::BindAsTexture(uint32_t slot)
    {
        m_Binding = slot;
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

    void GeneratorData::Resize(size_t size)
    {
        if (size == 0)
            return;
        const size_t texelCount  = size / sizeof(float);
        const int32_t resolution = static_cast<int32_t>(std::sqrt(static_cast<double>(texelCount)) + 0.5);
        if (resolution <= 0 || static_cast<size_t>(resolution) * resolution != texelCount)
            return;
        m_Size       = size;
        m_Resolution = resolution;
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Resolution, m_Resolution, 0, GL_RED, GL_FLOAT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GeneratorData::SetData(const void *data, size_t offset, size_t size)
    {
        if (size == 0)
            size = m_Size;
        if (m_Size < size || offset != 0 || size != m_Size)
            return;
        if (m_Size == 0)
            Resize(size);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Resolution, m_Resolution, GL_RED, GL_FLOAT, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    bool GeneratorData::CopyTo(const GeneratorData *other)
    {
        if (m_Size != other->m_Size)
            return false;
        if (m_Resolution != other->m_Resolution || m_InternalFormat != other->m_InternalFormat)
            return false;
        glCopyImageSubData(m_RendererID, GL_TEXTURE_2D, 0, 0, 0, 0,
                           other->m_RendererID, GL_TEXTURE_2D, 0, 0, 0, 0,
                           m_Resolution, m_Resolution, 1);
        return true;
    }

    float *GeneratorData::GetCPUCopy()
    {
        float *data = new float[m_Size / sizeof(float)];
        this->GetData(data, 0, m_Size);
        return data;
    }

    void GeneratorData::GetData(void *data, size_t offset, size_t size)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("readback/generator-data", PerformanceMonitor::Domain::Wait);
        TF3D_PROFILE_VALUE_DOMAIN("readback/generator-data-bytes", static_cast<uint64_t>(size), 0, 0,
                                  PerformanceMonitor::Domain::Wait);
        if (offset != 0 || size != m_Size)
            return;
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GeneratorData::SetDefaultStorage(GeneratorDataStorage storage)
    {
        s_DefaultStorage = storage;
        for (auto *instance : s_Instances)
            instance->SetStorage(storage);
    }

    GeneratorDataStorage GeneratorData::GetDefaultStorage()
    {
        return s_DefaultStorage;
    }

    const char *GeneratorData::GetDefaultStorageImageFormat()
    {
        return s_DefaultStorage == GeneratorDataStorage::R16F ? "r16f" : "r32f";
    }

    void GeneratorData::SetStorage(GeneratorDataStorage storage)
    {
        if (m_Storage == storage)
            return;
        float *cpuCopy = m_Size > 0 ? GetCPUCopy() : nullptr;
        glDeleteTextures(1, &m_RendererID);
        m_Storage        = storage;
        m_InternalFormat = m_Storage == GeneratorDataStorage::R16F ? GL_R16F : GL_R32F;
        m_PixelType      = m_Storage == GeneratorDataStorage::R16F ? GL_HALF_FLOAT : GL_FLOAT;
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (m_Resolution > 0)
            glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Resolution, m_Resolution, 0, GL_RED, GL_FLOAT, cpuCopy);
        glBindTexture(GL_TEXTURE_2D, 0);
        delete[] cpuCopy;
    }

    bool GeneratorData::SaveToFile(const std::string &path)
    {
        FILE *fd = fopen(path.c_str(), "wb");
        if (!fd) {
            return false;
        }

        float *data = GetCPUCopy();

        fwrite(s_GeneratorDataSaveFileHeader, sizeof(char), strlen(s_GeneratorDataSaveFileHeader), fd);

        fwrite(&m_Size, sizeof(size_t), 1, fd);
        fwrite(data, sizeof(float), m_Size / sizeof(float), fd);

        fclose(fd);
        delete[] data;

        return true;
    }

    bool GeneratorData::LoadFromFile(const std::string &path)
    {
        FILE *fd = fopen(path.c_str(), "rb");
        if (!fd) {
            return false;
        }

        char header[sizeof(s_GeneratorDataSaveFileHeader) + 1];

        fread(header, sizeof(char), strlen(s_GeneratorDataSaveFileHeader), fd);

        header[strlen(s_GeneratorDataSaveFileHeader)] = '\0';

        if (strcmp(header, s_GeneratorDataSaveFileHeader) != 0) {
            fclose(fd);
            return false;
        }

        size_t size = 0;
        fread(&size, sizeof(size_t), 1, fd);

        Resize(size);

        float *data = new float[size / sizeof(float)];
        fread(data, sizeof(float), size / sizeof(float), fd);
        SetData(data, 0, size);
        delete[] data;

        return true;
    }

} // namespace tf3d::generators
