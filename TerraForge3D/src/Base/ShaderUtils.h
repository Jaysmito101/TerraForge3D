#pragma once

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

int CompileShader(std::string shaderSrc, GLenum shaderType, std::string name);

void Log(const char* log);

void Log(std::string log);

int CreateProgram();