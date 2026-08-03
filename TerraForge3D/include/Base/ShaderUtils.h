#pragma once

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include "Base/Logging/Logger.h"

int CompileShader(std::string shaderSrc, GLenum shaderType, std::string name);

int CreateProgram();
