#pragma once
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

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

    class GLTFExporter
    {
    public:
        GLTFExporter();
        ~GLTFExporter();

        bool ExportGLTF(const std::string &path, const std::string &bin_path, Mesh *mesh, float *progress = nullptr);
        bool ExportGLB(const std::string &path, Mesh *mesh, float *progress = nullptr);

    private:
        void PrepareMeta(Mesh *mesh, const std::string &bin_path);
        void WriteBinaryData(Mesh *mesh, BinaryFileWriter *writer, float *progress);

    private:
        float m_Progress  = 0.0f;
        char buffer[4096] = {};
        nlohmann::json meta_data;
    };

} // namespace tf3d::exporters
using tf3d::exporters::GLTFExporter;
