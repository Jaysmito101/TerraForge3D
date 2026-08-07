#include "FrameBuffer.h"
#include "Profiler.h"

#include <glad/gl.h>

#include <algorithm>
#include <utility>

namespace tf3d::base
{

    FrameBuffer::FrameBuffer(int w, int h)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/framebuffer/allocate", PerformanceMonitor::Domain::Resource);
        width            = w;
        height           = h;
        GLint maxSamples = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        const GLsizei samples = std::max<GLsizei>(1, std::min(8, static_cast<GLsizei>(maxSamples)));
        TF3D_PROFILE_VALUE_DOMAIN("renderer/framebuffer/size", static_cast<uint64_t>(std::max(w, 0)),
                                  static_cast<uint64_t>(std::max(h, 0)), static_cast<uint64_t>(samples),
                                  PerformanceMonitor::Domain::Resource);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &multisampleColorTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multisampleColorTexture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA16F, w, h, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, multisampleColorTexture, 0);

        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depthTexture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH_COMPONENT24, w, h, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthTexture, 0);

        glGenFramebuffers(1, &resolveFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        glGenTextures(1, &resolvedDepthTexture);
        glBindTexture(GL_TEXTURE_2D, resolvedDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                               resolvedDepthTexture, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    FrameBuffer::~FrameBuffer()
    {
        glDeleteTextures(1, &colorTexture);
        glDeleteTextures(1, &multisampleColorTexture);
        glDeleteTextures(1, &depthTexture);
        glDeleteTextures(1, &resolvedDepthTexture);
        glDeleteFramebuffers(1, &fbo);
        glDeleteFramebuffers(1, &resolveFbo);
    }

    void FrameBuffer::Begin()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
    }

    void FrameBuffer::ResolveColor()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/resolve/color", PerformanceMonitor::Domain::Renderer);
        TF3D_PROFILE_GPU_SCOPE("renderer/resolve/color/gpu");
        TF3D_PROFILE_COUNTER_DOMAIN("gpu/framebuffer-resolves", 1.0, PerformanceMonitor::Domain::Gpu);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    void FrameBuffer::ResolveDepth()
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/resolve/depth", PerformanceMonitor::Domain::Renderer);
        TF3D_PROFILE_GPU_SCOPE("renderer/resolve/depth/gpu");
        TF3D_PROFILE_COUNTER_DOMAIN("gpu/framebuffer-resolves", 1.0, PerformanceMonitor::Domain::Gpu);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void FrameBuffer::Resolve()
    {
        ResolveColor();
        ResolveDepth();
    }

    uint32_t FrameBuffer::End()
    {
        Resolve();
        return colorTexture;
    }

    bool FrameBuffer::DownloadColorToU8(std::vector<uint8_t> &pixels, bool flipVertically)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("renderer/readback/viewport-color", PerformanceMonitor::Domain::Wait);
        pixels.clear();
        if (resolveFbo == 0 || width <= 0 || height <= 0)
            return false;

        ResolveColor();

        const size_t rowSize = static_cast<size_t>(width) * 3;
        std::vector<uint8_t> rawPixels(rowSize * static_cast<size_t>(height));

        GLint previousReadFramebuffer = 0;
        GLint previousDrawFramebuffer = 0;
        GLint previousPackAlignment   = 4;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
        glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, resolveFbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        TF3D_PROFILE_GPU_SCOPE("renderer/readback/viewport-color/gpu");
        TF3D_PROFILE_VALUE_DOMAIN("renderer/readback/viewport-color-bytes",
                                  static_cast<uint64_t>(rawPixels.size()), 0, 0,
                                  PerformanceMonitor::Domain::Wait);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rawPixels.data());

        glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);

        if (!flipVertically) {
            pixels = std::move(rawPixels);
            return true;
        }

        pixels.resize(rawPixels.size());
        for (int row = 0; row < height; ++row) {
            const size_t sourceOffset = static_cast<size_t>(height - row - 1) * rowSize;
            const size_t targetOffset = static_cast<size_t>(row) * rowSize;
            std::copy_n(rawPixels.data() + sourceOffset, rowSize, pixels.data() + targetOffset);
        }
        return true;
    }

    uint32_t FrameBuffer::GetColorTexture()
    {
        return colorTexture;
    }

    uint32_t FrameBuffer::GetDepthTexture()
    {
        return depthTexture;
    }

    uint32_t FrameBuffer::GetResolvedDepthTexture()
    {
        return resolvedDepthTexture;
    }

    uint32_t FrameBuffer::GetRendererID()
    {
        return fbo;
    }

} // namespace tf3d::base
