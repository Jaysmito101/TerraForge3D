#include "Shading/ShaderNodes/CustomShaderNode.h"

#include <iostream>
#include <sstream>
#include <regex>

static std::string ReplaceString(std::string subject, const std::string &search,
                                 const std::string &replace)
{
	size_t pos = 0;

	while((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}

	return subject;
}

void CustomShaderNode::OnEvaluate(GLSLFunction *function, GLSLLine *line)
{
	GLSLFunction ft = *func;
	std::string cline = code;
	// Add code symbols
	cline = ReplaceString(cline, "{ID}", STR(id));
	cline = ReplaceString(cline, "{OFFSET}", STR(dataBlobOffset));
	ft.AddLine(GLSLLine(cline));
	handler->AddFunction(ft);
	int i = 0;

	for(auto &param : params)
	{
		function->AddLine(param.first + " " + VAR(param.second + STR(i)) + ";");

		if(inputPins[i]->IsLinked())
		{
			GLSLLine tmp("");
			inputPins[i]->other->Evaluate(GetParams(function, &tmp));
			function->AddLine(VAR(param.second + STR(i)) + " = " + tmp.line + ";");
		}

		i++;
	}

	line->line = func->name + "(";
	i = 0;
	int n = params.size();

	for(auto &param : params)
	{
		line->line += VAR(param.second + STR(i));

		if(i < n - 1)
		{
			line->line += ", ";
		}

		i++;
	}

	line->line += ")";
}

void CustomShaderNode::Load(nlohmann::json data)
{
	int i = 0;

	for(auto &it : sharedDataTemplate)
	{
		if(it.first == "float")
		{
			fData[i] = data[it.second].get<float>();
		}

		else if(it.first == "bool")
		{
			bData[i] = data[it.second].get<bool>();
		}

		i++;
	}
}

nlohmann::json CustomShaderNode::Save()
{
	nlohmann::json data;
	data["type"] = "CustomShader";
	int i = 0;

	for(auto &it : sharedDataTemplate)
	{
		if(it.first == "float")
		{
			data[it.second] = fData[i];
		}

		else if(it.first == "bool")
		{
			data[it.second] = bData[i];
		}

		i++;
	}

	data["shader"] = shader;
	return data;
}

void CustomShaderNode::UpdateShaders()
{
	int i = 0;

	for(auto &it : sharedDataTemplate)
	{
		if(i > 31)
		{
			break;
		}

		if(it.second == "float")
		{
			SetSharedMemoryItem(sharedData, i, fData[i]);
		}

		else if(it.second == "bool")
		{
			SetSharedMemoryItem(sharedData, i, bData[i] ? 1.0f : 0.0f);
		}

		i++;
	}
}

void CustomShaderNode::OnRender()
{
	DrawHeader(name);
	ImGui::Dummy(ImVec2(120, 10));
	ImGui::SameLine();
	outputPins[0]->Render();
	ImGui::NewLine();
	int j = 0;

	for(auto &params: params)
	{
		inputPins[j]->Render();
		ImGui::Text(params.second.c_str());
		j++;
	}

	ImGui::PushItemWidth(100);
	int i = 0;

	for(auto &it : sharedDataTemplate)
	{
		if(i > 31)
		{
			break;
		}

		ImGui::PushID(i);

		if(it.second == "float")
		{
			if(ImGui::DragFloat(it.first.c_str(), &fData[i], 0.01f))
			{
				SetSharedMemoryItem(sharedData, i, fData[i]);
			}
		}

		else if(it.second == "bool")
		{
			if(ImGui::Checkbox(it.first.c_str(), &bData[i]))
			{
				SetSharedMemoryItem(sharedData, i, bData[i] ? 1.0f : 0.0f);
			}
		}

		ImGui::PopID();
		i++;
	}

	ImGui::PopItemWidth();
}

CustomShaderNode::CustomShaderNode(GLSLHandler *handler, std::string s)
	:SNENode(handler)
{
	name = "Custom Shader";
	headerColor = ImColor(SHADER_VALUE_NODE_COLOR);
	shader = s;
	// Parse the shader
	std::stringstream ss(shader);
	std::string line;
	std::string codeStr = "";
	std::string metaStr = "";
	bool isCodeActive = false;
	bool isMetaActive = false;

	while(std::getline(ss, line))
	{
		if(line[0] == '#')
		{
			continue;
		}

		else if(line == "[META]")
		{
			isMetaActive = true;
		}

		else if(line == "[CODE]")
		{
			isCodeActive = true;
		}

		else if(line == "[/META]")
		{
			isMetaActive = false;
		}

		else if(line == "[/CODE]")
		{
			isCodeActive = false;
		}

		else if(isMetaActive)
		{
			metaStr += line + "\n";
		}

		else if(isCodeActive)
		{
			codeStr += line + "\n";
		}
	}

	// Load the meta data
	if(metaStr.size() > 0)
	{
		meta = nlohmann::json::parse(metaStr);
	}

	else
	{
		throw std::runtime_error("No meta data found in shader");
	}

	// Load the code
	if(codeStr.size() > 0)
	{
		code = codeStr;
	}

	else
	{
		throw std::runtime_error("No code found in shader");
	}

	// Load the data
	name = meta["name"];
	std::cout << "Loading Node : " << name << "\n";
	std::string paramsStr = "";
	int j = 0;
	int pss = meta["params"].size();

	for(std::string param : meta["params"])
	{
		std::string pType = param.substr(0, param.find(":"));
		std::string pName = param.substr(param.find(":") + 1);
		params.push_back(std::make_pair(pType, pName));
		paramsStr += pType + " " + pName + (j < pss - 1 ? ", " : "");
		j++;
	}

	func = new GLSLFunction(meta["fname"], paramsStr, meta["returns"]);
	sharedDataTemplate.clear();
	int i = 0;

	for(auto it = meta["sharedData"].begin() ; it != meta["sharedData"].end() ; it++)
	{
		sharedDataTemplate.push_back(std::make_pair(it.key(), it.value()["type"]));

		if(it.value()["type"] == "float")
		{
			fData[i] = it.value()["default"].get<float>();
		}

		else if(it.value()["type"] == "bool")
		{
			bData[i] = it.value()["default"].get<bool>();
		}

		i++;
	}

	if(meta["returns"] == "float")
	{
		outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float));
	}

	else if(meta["returns"] == "vec3")
	{
		outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float3));
	}

	else
	{
		throw std::runtime_error("Custom shader node does not support return type " + meta["returns"]);
	}

	for(auto &it : params)
	{
		if(it.first == "float")
		{
			inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float));
		}

		else if(it.first == "vec3")
		{
			inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3));
		}

		else
		{
			throw std::runtime_error("Custom shader node does not support parameter type " + it.first);
		}
	}

	headerColor = ImColor(meta["color"]["r"].get<int>(), meta["color"]["r"].get<int>(), meta["color"]["b"].get<int>());
}


CustomShaderNode::~CustomShaderNode()
{
	delete func;
}