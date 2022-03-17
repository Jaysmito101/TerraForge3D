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
	int did = 0;
	for(auto it : sharedDataTemplate)
	{
		cline = ReplaceString(cline, it.alias, SDATA(did));
		did++;
	}
	ft.AddLine(GLSLLine(cline));
	handler->AddFunction(ft);
	int i = 0;

	if(!useArrayParams)
	{
		i = 0;
		for(auto &param : params)
		{
			function->AddLine(GLSLLine(param.first + " " + VAR(param.second + STR(i)) + " = " + param.first + "(0.0f);"));
	
			if(inputPins[i]->IsLinked())
			{
				GLSLLine tmp("");
				inputPins[i]->other->Evaluate(GetParams(function, &tmp));
				function->AddLine(GLSLLine(VAR(param.second + STR(i)) + " = " + tmp.line + ";"));
			}
	
			i++;
		}
	}
	else
	{	
		i = 0;
		for(auto &param: params)
		{
			function->AddLine(GLSLLine(param.first + " " + VAR(param.second + STR(i)) + "[" + STR(paramCount) + "];"));
			for(int k = 0 ; k < paramCount ; k++)		
			{
				if (inputPins[k * params.size() + i]->IsLinked())
				{
					GLSLLine tmp("");
					inputPins[k * params.size() + i]->other->Evaluate(GetParams(function, &tmp));
					function->AddLine(GLSLLine(VAR(param.second + STR(i)) + "[" + STR(k) + "]" + " = " + tmp.line + ";"));
				}
				else
				{
					function->AddLine(GLSLLine(VAR(param.second + STR(i)) + "[" + STR(k) + "]" + " = " + param.first + "(0.0f);"));
				}
			}
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
	int callerPin = 0;
	for(int i = 0;i<outputPins.size();i++)
	{
		if(outputPins[i]->id == callerPinId)
		{
			callerPin = i;
			break;
		}
	}
	line->line += (params.size() == 0 ? "" : ", ") + STR(callerPin) + ")";
}

void CustomShaderNode::Load(nlohmann::json data)
{
	int i = 0;

	for(auto &it : sharedDataTemplate)
	{
		if(it.type == "float")
		{
			fData[i] = data[it.alias].get<float>();
		}

		else if(it.type == "bool")
		{
			bData[i] = data[it.alias].get<bool>();
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
		if(it.type == "float")
		{
			data[it.alias] = fData[i];
		}

		else if(it.type == "bool")
		{
			data[it.alias] = bData[i];
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

		if(it.type == "float")
		{
			SetSharedMemoryItem(sharedData, i, fData[i]);
		}

		else if(it.type == "bool")
		{
			SetSharedMemoryItem(sharedData, i, bData[i] ? 1.0f : 0.0f);
		}

		i++;
	}
}

void CustomShaderNode::OnRender()
{
	DrawHeader(name);

	if(!useMultipleOpins)
	{
		ImGui::Text("Output");
		outputPins[0]->Render();
		ImGui::NewLine();
	}
	else
	{
		for(int i = 0 ; i < oPinStrs.size() ; i++)
		{
			ImGui::Text(oPinStrs[i].data());
			outputPins[i]->Render();
			ImGui::NewLine();
		}
	}

	int j = 0;

	if(!useArrayParams)
	{
		for(auto &params: params)
		{
			inputPins[j]->Render();
			ImGui::Text(params.second.c_str());
			j++;
		}
	}
	else
	{
		for(int k = 0 ; k < paramCount ; k++)	
		{
			j = 0;
			for(auto &param: params)
			{
				inputPins[k + j]->Render();
				ImGui::Text((param.second + " #" + STR(k)).c_str());
				j++;
			}
		}
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

		if(it.type == "float")
		{
			if(ImGui::DragFloat(it.text.data(), &fData[i], 0.01f))
			{
				SetSharedMemoryItem(sharedData, i, fData[i]);
			}
		}

		else if(it.type == "bool")
		{
			if(ImGui::Checkbox(it.text.data(), &bData[i]))
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
	
	if(meta.find("param_count") != meta.end())
	{
		paramCount = meta["param_count"];
		useArrayParams = true;
	}
	else
	{
		paramCount = 1;
		useArrayParams = false;	
	}
	int pss = meta["params"].size();

	for(std::string param : meta["params"])
	{
		std::string pType = param.substr(0, param.find(":"));
		std::string pName = param.substr(param.find(":") + 1);
		params.push_back(std::make_pair(pType, pName));
		if(useArrayParams)
			paramsStr += pType + " " + pName + "[" + STR(paramCount) + "]" + (j < pss - 1 ? ", " : "");
		else
			paramsStr += pType + " " + pName + (j < pss - 1 ? ", " : "");
		j++;
	}

	paramsStr += (params.size() == 0 ? "" : ", ") + std::string("int callerPin");

	func = new GLSLFunction(meta["fname"], paramsStr, meta["returns"]);
	func->name += STR(id);
	sharedDataTemplate.clear();
	int i = 0;

	for(auto it = meta["sharedData"].begin() ; it != meta["sharedData"].end() ; it++)
	{
		std::string alias = ReplaceString(it.key(), " ", "");
		if(it.value().find("alias") != it.value().end())
		    alias = it.value()["alias"];
		sharedDataTemplate.push_back(SharedDataRep(it.key(), it.value()["type"], alias));

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

	if(meta.find("output_pins") != meta.end())
	{
		std::string returns = meta["returns"];
		useMultipleOpins = true;
		for(std::string oPin : meta["output_pins"])
		{
			oPinStrs.push_back(oPin);
			if(meta["returns"] == "float")
			{
				outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float));
			}
			else if(meta["returns"] == "vec3")
			{
				outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float3));
			}
		}
	}
	else
	{
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
	}


	for(int k = 0 ; k < paramCount ; k++)	
	{
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
	}
	headerColor = ImColor(meta["color"]["r"].get<int>(), meta["color"]["r"].get<int>(), meta["color"]["b"].get<int>());
}


CustomShaderNode::~CustomShaderNode()
{
	delete func;
}
