#pragma once

#include <string>

namespace tf3d::base
{

    void ExportTexture(int fbo, std::string path, int w, int h);

} // namespace tf3d::base
using tf3d::base::ExportTexture;