#pragma once
#include <sstream>
#include <string>

namespace tf3d::exporters
{

    class RawTextureExporter
    {
    public:
        RawTextureExporter();
        ~RawTextureExporter();

        bool ExportHeightmap(const std::string &path, float *data, int bitDepth, int resolution, float *progress = nullptr);

    private:
    private:
        float m_Progress  = 0.0f;
        char buffer[4096] = {};
    };
} // namespace tf3d::exporters
using tf3d::exporters::RawTextureExporter;
