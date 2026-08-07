#include "ExportTexture.h"
#include "Profiler.h"

#include <glad/gl.h>
#include <stb/stb_image_write.h>

namespace tf3d::base
{

    void ExportTexture(int fbo, std::string path, int w, int h)
    {
        TF3D_PROFILE_SCOPE_DOMAIN("export/viewport", PerformanceMonitor::Domain::Io);
        if (path.size() < 3) {
            return;
        }

        if (path.find(".png") == std::string::npos) {
            path += ".png";
        }

        unsigned char *data = new unsigned char[w * h * 3];
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        {
            TF3D_PROFILE_SCOPE_DOMAIN("export/viewport/readback", PerformanceMonitor::Domain::Wait);
            TF3D_PROFILE_VALUE_DOMAIN("readback/export-bytes", static_cast<uint64_t>(w) * static_cast<uint64_t>(h) * 3ull,
                                      static_cast<uint64_t>(w), static_cast<uint64_t>(h),
                                      PerformanceMonitor::Domain::Wait);
            TF3D_PROFILE_GPU_SCOPE("export/viewport/readback/gpu");
            glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        stbi_flip_vertically_on_write(true);
        stbi_write_png(path.c_str(), w, h, 3, data, w * 3);
        delete[] data;
    }

} // namespace tf3d::base
