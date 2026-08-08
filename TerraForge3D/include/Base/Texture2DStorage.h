#pragma once

#include <glad/gl.h>
#include <cstdint>

namespace tf3d::base
{
    class Texture2DStorage
    {
    public:
        Texture2DStorage() = default;
        ~Texture2DStorage();

        Texture2DStorage(const Texture2DStorage &)            = delete;
        Texture2DStorage &operator=(const Texture2DStorage &) = delete;

        bool Allocate(int32_t width, int32_t height, int32_t mipLevels, uint32_t internalFormat,
                      uint32_t minFilter, uint32_t magFilter,
                      uint32_t wrapS = GL_CLAMP_TO_EDGE, uint32_t wrapT = GL_CLAMP_TO_EDGE);
        void Release();

        void Bind(uint32_t slot = 0) const;
        void BindImage(uint32_t imageUnit, uint32_t access, uint32_t imageFormat, int32_t level = 0) const;

        inline uint32_t GetRendererID() const
        {
            return m_RendererID;
        }

        inline int32_t GetWidth() const
        {
            return m_Width;
        }

        inline int32_t GetHeight() const
        {
            return m_Height;
        }

        inline bool IsValid() const
        {
            return m_RendererID != 0;
        }

    private:
        uint32_t m_RendererID     = 0;
        int32_t m_Width           = 0;
        int32_t m_Height          = 0;
        int32_t m_MipLevels       = 0;
        uint32_t m_InternalFormat = 0;
        uint32_t m_MinFilter     = 0;
        uint32_t m_MagFilter     = 0;
        uint32_t m_WrapS         = 0;
        uint32_t m_WrapT         = 0;
    };
} // namespace tf3d::base

using tf3d::base::Texture2DStorage;
