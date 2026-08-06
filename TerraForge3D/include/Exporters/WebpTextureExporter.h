#pragma once
#include <sstream>
#include <string>

#include <webp/encode.h>

namespace tf3d::exporters
{

    class WebpTextureExporter
    {
    public:
        WebpTextureExporter();
        ~WebpTextureExporter();

        bool ExportHeightmap(const std::string &path, float *data, int bitDepth, int resolution, float *progress = nullptr);

    private:
    private:
        float m_Progress  = 0.0f;
        char buffer[4096] = {};
    };
} // namespace tf3d::exporters
using tf3d::exporters::WebpTextureExporter;
