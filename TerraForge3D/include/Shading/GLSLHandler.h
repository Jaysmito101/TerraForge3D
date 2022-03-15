#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct GLSLUniform
{
	GLSLUniform(std::string name, std::string type, std::string value = "");
	~GLSLUniform();

	std::string GenerateGLSL();

	std::string name = "";
	std::string type = "";
	std::string value = "";
	std::string comment = "";
};

struct GLSLMacro
{
	GLSLMacro(std::string name, std::string value, std::string comment = "");
	~GLSLMacro();

	std::string GenerateGLSL();

	std::string name = "";
	std::string value = "";
	std::string comment = "";
};

struct GLSLLine
{
	GLSLLine(std::string line, std::string comment = "");
	~GLSLLine();

	std::string GenerateGLSL();

	std::string line = "";
	std::string comment = "";
};

struct GLSLSSBO
{
	GLSLSSBO(std::string name, std::string binding = "1", std::string comment = "");
	~GLSLSSBO();

	std::string GenerateGLSL();

	void AddLine(GLSLLine line);

	std::vector<GLSLLine> lines;
	std::string name = "";
	std::string binding = "";
	std::string comment = "";
};

struct GLSLFunction
{
	GLSLFunction(std::string name, std::string params = "", std::string returnType = "void");
	~GLSLFunction();

	std::string GenerateGLSL();

	void AddLine(GLSLLine line);

	std::string name = "";
	std::string returnType = "";
	std::string params = "";
	std::string comment = "";
	std::vector<GLSLLine> lines;
};

class GLSLHandler
{
public:
	GLSLHandler(std::string name = "Shader");
	~GLSLHandler();

	std::string GenerateGLSL();

	void AddTopLine(GLSLLine line);
	void AddUniform(GLSLUniform uniform);
	void AddMacro(GLSLMacro macro);
	void AddSSBO(GLSLSSBO ssbo);
	void AddFunction(GLSLFunction function);

	bool HasFunction(std::string name);

	void Clear();

public:
	std::string version = "430 core";
	std::string code = "";
	std::string name = "";
	std::vector<GLSLUniform> uniforms;
	std::vector<GLSLFunction> functions;
	std::vector<std::string> functionNames;
	std::vector<GLSLMacro> macros;
	std::vector<GLSLSSBO> ssbos;
	std::vector<GLSLLine> topLines;
};