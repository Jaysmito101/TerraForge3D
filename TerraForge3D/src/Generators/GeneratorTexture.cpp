#include "Generators/GeneratorTexture.h"
#include "Profiler.h"

#include <algorithm>

namespace tf3d::generators
{

    GeneratorTexture::GeneratorTexture(int32_t width, int32_t height, GeneratorTextureStorage storage)
    {
        m_Width   = width;
        m_Height  = height;
        m_Storage = storage;
        if (m_Storage == GeneratorTextureStorage::R8) {
            m_Format         = GL_RED;
            m_InternalFormat = GL_R8;
            m_PixelType      = GL_UNSIGNED_BYTE;
        } else if (m_Storage == GeneratorTextureStorage::R16) {
            m_Format         = GL_RED;
            m_InternalFormat = GL_R16;
            m_PixelType      = GL_UNSIGNED_SHORT;
        } else if (m_Storage == GeneratorTextureStorage::R16F) {
            m_Format         = GL_RED;
            m_InternalFormat = GL_R16F;
            m_PixelType      = GL_HALF_FLOAT;
        } else if (m_Storage == GeneratorTextureStorage::RG32F) {
            m_Format         = GL_RG;
            m_InternalFormat = GL_RG32F;
            m_PixelType      = GL_FLOAT;
        } else {
            m_Format         = GL_RGBA;
            m_InternalFormat = GL_RGBA32F;
            m_PixelType      = GL_FLOAT;
        }
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_Format, m_PixelType, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (IsSingleChannel()) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        }
        ClearTexture();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GeneratorTexture::~GeneratorTexture()
    {
        glDeleteTextures(1, &m_RendererID);
    }

    void GeneratorTexture::Resize(int32_t width, int32_t height)
    {
        if (width == m_Width && height == m_Height)
            return;
        if (width == 0 || height == 0)
            return;
        m_Width  = width;
        m_Height = height;
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_Format, m_PixelType, nullptr);
        ClearTexture();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GeneratorTexture::GenerateMipmaps()
    {
        if (m_RendererID == 0 || m_Width <= 0 || m_Height <= 0)
            return;

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                        GL_TEXTURE_UPDATE_BARRIER_BIT |
                        GL_TEXTURE_FETCH_BARRIER_BIT);

        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        const int32_t largestDimension = std::max(m_Width, m_Height);
        int32_t mipLevels              = 1;
        for (int32_t mipSize = largestDimension; mipSize > 1; mipSize >>= 1)
            ++mipLevels;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    float *GeneratorTexture::MakeCPUCopy()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("readback/generator-texture", PerformanceMonitor::Domain::Wait);
        const int channels = GetChannelCount();
        m_Data             = new float[m_Width * m_Height * channels];
        TF3D_PROFILE_VALUE_DOMAIN("readback/generator-texture-bytes",
                                  static_cast<uint64_t>(m_Width) * static_cast<uint64_t>(m_Height) *
                                      static_cast<uint64_t>(channels) * sizeof(float),
                                  static_cast<uint64_t>(m_Width), static_cast<uint64_t>(m_Height),
                                  PerformanceMonitor::Domain::Wait);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glGetTexImage(GL_TEXTURE_2D, 0, m_Format, GL_FLOAT, m_Data);
        glBindTexture(GL_TEXTURE_2D, 0);
        return m_Data;
    }

    void GeneratorTexture::FreeCPUCopy()
    {
        if (m_Data != nullptr) {
            delete[] m_Data;
            m_Data = nullptr;
        }
    }

    void GeneratorTexture::UploadCPUCopy()
    {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (IsSingleChannel()) {
            if (m_Storage == GeneratorTextureStorage::R16) {
                std::vector<uint16_t> values(m_Width * m_Height);
                for (size_t i = 0; i < values.size(); i++)
                    values[i] = static_cast<uint16_t>(glm::clamp(m_Data[i], 0.0f, 1.0f) * 65535.0f + 0.5f);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, m_PixelType, values.data());
            } else if (m_Storage == GeneratorTextureStorage::R16F) {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, GL_FLOAT, m_Data);
            } else {
                std::vector<uint8_t> values(m_Width * m_Height);
                for (size_t i = 0; i < values.size(); i++)
                    values[i] = static_cast<uint8_t>(glm::clamp(m_Data[i], 0.0f, 1.0f) * 255.0f + 0.5f);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, m_PixelType, values.data());
            }
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_Format, m_PixelType, m_Data);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GeneratorTexture::ZeroCPUCopy()
    {
        if (m_Data != nullptr) {
            const int channels = GetChannelCount();
            memset(m_Data, 0, m_Width * m_Height * channels * sizeof(float));
        }
    }

    int32_t GeneratorTexture::Bind(int32_t slot)
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        return slot;
    }

    int32_t GeneratorTexture::BindForCompute(int32_t binding)
    {
        // glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glBindImageTexture(binding, m_RendererID, 0, GL_FALSE, 0, GL_READ_WRITE, m_InternalFormat);
        return binding;
    }

    void GeneratorTexture::SetPixel(float x, float y, float r, float g, float b, float a)
    {
        int32_t pixelX     = std::clamp((int32_t)(x * m_Width), 0, m_Width - 1);
        int32_t pixelY     = std::clamp((int32_t)(y * m_Height), 0, m_Height - 1);
        const int channels = GetChannelCount();
        if (m_Data == nullptr) {
            glBindTexture(GL_TEXTURE_2D, m_RendererID);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            if (IsSingleChannel()) {
                if (m_Storage == GeneratorTextureStorage::R16) {
                    const uint16_t data = static_cast<uint16_t>(glm::clamp(r, 0.0f, 1.0f) * 65535.0f + 0.5f);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, m_PixelType, &data);
                } else if (m_Storage == GeneratorTextureStorage::R16F) {
                    const float data = r;
                    glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, GL_FLOAT, &data);
                } else {
                    const uint8_t data = static_cast<uint8_t>(glm::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, m_PixelType, &data);
                }
            } else {
                float data[] = {r, g, b, a};
                glTexSubImage2D(GL_TEXTURE_2D, 0, pixelX, pixelY, 1, 1, m_Format, m_PixelType, data);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        } else {
            if (IsSingleChannel()) {
                m_Data[pixelY * m_Width + pixelX] = r;
            } else {
                const size_t offset = static_cast<size_t>(pixelY * m_Width + pixelX) * channels;
                m_Data[offset + 0]  = r;
                if (channels > 1)
                    m_Data[offset + 1] = g;
                if (channels > 2)
                    m_Data[offset + 2] = b;
                if (channels > 3)
                    m_Data[offset + 3] = a;
            }
        }
    }

    int32_t GeneratorTexture::GetChannelCount() const
    {
        return IsSingleChannel() ? 1 : (IsTwoChannel() ? 2 : 4);
    }

    void GeneratorTexture::ClearTexture()
    {
        if (IsSingleChannel()) {
            if (m_Storage == GeneratorTextureStorage::R16) {
                const uint16_t zero = 0;
                glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, &zero);
            } else if (m_Storage == GeneratorTextureStorage::R16F) {
                const float zero = 0.0f;
                glClearTexImage(m_RendererID, 0, m_Format, GL_FLOAT, &zero);
            } else {
                const uint8_t zero = 0;
                glClearTexImage(m_RendererID, 0, m_Format, m_PixelType, &zero);
            }
            return;
        }

        const float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        glClearTexImage(m_RendererID, 0, m_Format, GL_FLOAT, zero);
    }

} // namespace tf3d::generators
