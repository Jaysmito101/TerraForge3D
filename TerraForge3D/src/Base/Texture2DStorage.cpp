#include "Base/Texture2DStorage.h"

#include <glad/gl.h>

namespace tf3d::base
{
    Texture2DStorage::~Texture2DStorage()
    {
        Release();
    }

    bool Texture2DStorage::Allocate(int32_t width, int32_t height, int32_t mipLevels, uint32_t internalFormat,
                                     uint32_t minFilter, uint32_t magFilter, uint32_t wrapS, uint32_t wrapT)
    {
        if (width <= 0 || height <= 0 || mipLevels <= 0 || internalFormat == 0)
            return false;

        if (m_RendererID != 0 && m_Width == width && m_Height == height && m_MipLevels == mipLevels &&
            m_InternalFormat == internalFormat && m_MinFilter == minFilter && m_MagFilter == magFilter &&
            m_WrapS == wrapS && m_WrapT == wrapT) {
            return true;
        }

        Release();
        GLint previousActiveTexture = GL_TEXTURE0;
        GLint previousTexture        = 0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexStorage2D(GL_TEXTURE_2D, mipLevels, internalFormat, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glActiveTexture(previousActiveTexture);

        m_Width           = width;
        m_Height          = height;
        m_MipLevels       = mipLevels;
        m_InternalFormat  = internalFormat;
        m_MinFilter       = minFilter;
        m_MagFilter       = magFilter;
        m_WrapS           = wrapS;
        m_WrapT           = wrapT;
        return true;
    }

    void Texture2DStorage::Release()
    {
        if (m_RendererID != 0)
            glDeleteTextures(1, &m_RendererID);

        m_RendererID     = 0;
        m_Width          = 0;
        m_Height         = 0;
        m_MipLevels      = 0;
        m_InternalFormat = 0;
        m_MinFilter      = 0;
        m_MagFilter      = 0;
        m_WrapS          = 0;
        m_WrapT          = 0;
    }

    void Texture2DStorage::Bind(uint32_t slot) const
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

    void Texture2DStorage::BindImage(uint32_t imageUnit, uint32_t access, uint32_t imageFormat, int32_t level) const
    {
        glBindImageTexture(imageUnit, m_RendererID, level, GL_FALSE, 0, access, imageFormat);
    }
} // namespace tf3d::base
