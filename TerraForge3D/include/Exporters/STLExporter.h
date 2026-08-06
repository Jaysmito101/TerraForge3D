#pragma once
#include <sstream>
#include <string>

namespace tf3d::base
{
    class Mesh;
}
using tf3d::base::Mesh;
namespace tf3d::base
{
    class BinaryFileWriter;
}
using tf3d::base::BinaryFileWriter;

namespace tf3d::exporters
{

    class STLExporter
    {
    public:
        STLExporter();
        ~STLExporter();

        bool ExportASCII(const std::string &path, Mesh *mesh, float *progress = nullptr);
        bool ExportBinary(const std::string &path, Mesh *mesh, float *progress = nullptr);

    private:
        float m_Progress  = 0.0f;
        char buffer[4096] = {};
    };
} // namespace tf3d::exporters
using tf3d::exporters::STLExporter;
