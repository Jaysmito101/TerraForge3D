#pragma once

#include "Base/Logging/Logger.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

int CompileShader(std::string shaderSrc, GLenum shaderType, std::string name);

int CreateProgram();
