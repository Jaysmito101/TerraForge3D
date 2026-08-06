#pragma once
#include <sstream>
#include <string>

namespace tf3d::exporters
{

    class ExrTextureExporter
    {
    public:
        ExrTextureExporter();
        ~ExrTextureExporter();

        bool ExportHeightmap(const std::string &path, float *data, int bitDepth, int resolution, float *progress = nullptr);

    private:
        bool SaveExr(const std::string &path, int resolution, int channels, int bitDepth, float *ch0_ptr, float *ch1_ptr = nullptr, float *ch2_ptr = nullptr);

    private:
        float m_Progress  = 0.0f;
        char buffer[4096] = {};
    };
} // namespace tf3d::exporters
using tf3d::exporters::ExrTextureExporter;
