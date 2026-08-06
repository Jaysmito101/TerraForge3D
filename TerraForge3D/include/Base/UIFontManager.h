#pragma once

#include "imgui.h"
#include <string>

namespace tf3d::base
{

    void LoadUIFont(std::string name, float pixelSize, std::string path);
    ImFont *GetUIFont(std::string name);

} // namespace tf3d::base
using tf3d::base::GetUIFont;
using tf3d::base::LoadUIFont;
