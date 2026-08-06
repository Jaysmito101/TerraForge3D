#pragma once

#include "Base/Logging/Logger.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace tf3d::base
{

    int CompileShader(std::string shaderSrc, GLenum shaderType, std::string name);

    int CreateProgram();

} // namespace tf3d::base
using tf3d::base::CompileShader;
using tf3d::base::CreateProgram;
