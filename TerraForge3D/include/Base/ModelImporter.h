#pragma once

#include "Base/Model.h"
#include <string>

namespace tf3d::base
{

    Model *LoadModel(std::string path);

} // namespace tf3d::base
using tf3d::base::LoadModel;
