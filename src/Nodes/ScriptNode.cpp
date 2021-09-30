#include <NodeEditorNodes.h>
#include <LuaNativeFunctions.h>

static std::string floatNodeDefaultScript = R"(
function Evaluate(x, y, a, b, c)
    return x + y
end
)";


// ---------------------------------------------------------------------------------------------------------
// -------SCRIPT-FLOAT-NODE-BEGIN---------------------------------------------------------------------------


static std::vector<std::pair<int, std::string>> *writeBlock;

int WriteFunction(lua_State* L) {
	if(!writeBlock)
		return -1;
	std::string s = std::string(luaL_checkstring(L, 1));
	writeBlock->push_back(std::make_pair(0, s));
	return 1;
}

void ScriptFloatNode::Setup() {
	outputPin.node = this;
	inputPinV.node = this;
	inputPinX.node = this;
	inputPinY.node = this;
	L = luaL_newstate();
	luaL_openlibs(L);
	outputTex = new Texture2D(*data.resolution, *data.resolution);
	editor = new TextEditor();
	auto lang = TextEditor::LanguageDefinition::Lua();
	editor->SetLanguageDefinition(lang);
	editor->SetText(floatNodeDefaultScript);
}

std::vector<void*> ScriptFloatNode::GetPins() {
	return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &outputPin });
}

float ScriptFloatNode::EvaluatePin(float x, float y, int id) {
	if (isCompiled) {
			lua_getglobal(L, "Evaluate");
			lua_pushnumber(L, x);
			lua_pushnumber(L, y);
			float vv = a;
			if (inputPinV.isLinked)
				vv = inputPinV.Evaluate(0, 0);
			lua_pushnumber(L, vv);
			vv = b;
			if (inputPinX.isLinked)
				vv = inputPinX.Evaluate(0, 0);
			lua_pushnumber(L, vv);
			vv = c;
			if (inputPinY.isLinked)
				vv = inputPinY.Evaluate(0, 0);
			lua_pushnumber(L, vv);
			if (lua_pcall(L, 5, 1, 0) == LUA_OK) {
				if (lua_isnumber(L, -1)) {
					double result = lua_tonumber(L, -1);
					lua_pop(L, 1);
					return (float)result;
				}
				lua_pop(L, lua_gettop(L));
			}
	}
	return 0.0f;
}

nlohmann::json ScriptFloatNode::Save() {
	nlohmann::json data;
	data["type"] = NodeType::ScriptF;
	data["inputPinZ"] = inputPinV.Save();
	data["inputPinX"] = inputPinX.Save();
	data["inputPinY"] = inputPinY.Save();
	data["outputPin"] = outputPin.Save();
	data["a"] = a;
	data["b"] = b;
	data["c"] = c;
	data["id"] = id;
	data["name"] = name;
	data["isDraggable"] = isDraggable;
	data["showEditor"] = showEditor;
	data["showConsole"] = showConsole;
	data["showTexture"] = showTexture;
	return data;
}

void ScriptFloatNode::Load(nlohmann::json data) {
	inputPinV.Load(data["inputPinV"]);
	inputPinX.Load(data["inputPinX"]);
	inputPinY.Load(data["inputPinY"]);
	outputPin.Load(data["outputPin"]);
	a = data["a"];
	b = data["b"];
	c = data["c"];
	id = data["id"];
	name = data["name"];
	isDraggable = data["isDraggable"];
	showEditor = data["showEditor"];
	showConsole = data["showConsole"];
	showTexture = data["showTexture"];
}

bool ScriptFloatNode::Render() {
	writeBlock = &output;


	ImNodes::BeginNode(id);

	ImNodes::BeginNodeTitleBar();
	ImGui::Text(name.c_str());
	ImNodes::EndNodeTitleBar();

	ImNodes::BeginOutputAttribute(outputPin.id);
	ImNodes::EndOutputAttribute();

	if (!inputPinV.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::PushItemWidth(300);
		ImGui::DragFloat((std::string("Value a##clampNode") + std::to_string(inputPinV.id)).c_str(), &a, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinV.id);
		ImGui::Dummy(ImVec2(300, 20));
		ImNodes::EndInputAttribute();
	}

	if (!inputPinX.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinX.id);
		ImGui::PushItemWidth(300);
		ImGui::DragFloat((std::string("Value b##clampNode") + std::to_string(inputPinX.id)).c_str(), &b, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinX.id);
		ImGui::Dummy(ImVec2(300, 20));
		ImNodes::EndInputAttribute();
	}

	if (!inputPinY.isLinked)
	{
		ImNodes::BeginInputAttribute(inputPinY.id);
		ImGui::PushItemWidth(300);
		ImGui::DragFloat((std::string("Value c##clampNode") + std::to_string(inputPinY.id)).c_str(), &c, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndInputAttribute();
	}
	else {
		ImNodes::BeginInputAttribute(inputPinY.id);
		ImGui::Dummy(ImVec2(300, 20));
		ImNodes::EndInputAttribute();
	}


	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
	if (isCompiled) {
		if (ImGui::Button("Simulate")) {
			lua_getglobal(L, "Evaluate");
			lua_pushnumber(L, 0);
			lua_pushnumber(L, 0);
			float vv = a;
			if (inputPinV.isLinked)
				vv = inputPinV.Evaluate(0, 0);
			lua_pushnumber(L, vv);
			vv = b;
			if (inputPinX.isLinked)
				vv = inputPinX.Evaluate(0, 0);
			lua_pushnumber(L, vv);
			vv = c;
			if (inputPinY.isLinked)
				vv = inputPinY.Evaluate(0, 0);
			lua_pushnumber(L, vv);
			if (lua_pcall(L, 5, 1, 0) == LUA_OK) {
				if (lua_isnumber(L, -1)) {
					double result = lua_tonumber(L, -1);
					lua_pop(L, 1);
					output.push_back(std::make_pair(0, std::to_string(result)));
				}
				else {
					output.push_back(std::make_pair(1, "Return value is not number!"));
				}
				lua_pop(L, lua_gettop(L));
			}else{
				output.push_back(std::make_pair(1, "Function call to Evaluate failed!"));
			}
		}
	}
	else {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_Button]);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyle().Colors[ImGuiCol_Button]);
		ImGui::Button("Execute");
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	ImGui::SameLine();


	if (ImGui::Button("Compile")) {
		std::string tmp = editor->GetText();
		int result;
		if(tmp.size() > 0)
			result = luaL_dostring(L, tmp.c_str());
		else
			result = luaL_dostring(L, floatNodeDefaultScript.c_str());
		if (result == LUA_OK) {
			lua_pop(L, lua_gettop(L));
			lua_pushcfunction(L, WriteFunction);
			lua_setglobal(L, "Print");
			LoadLuaNAtiveFunctions(L);
			isCompiled = true;
			output.push_back(std::make_pair(2, "Successfully Compiled!"));
		}
		else {
			output.push_back(std::make_pair(1, "Compilation Failed!"));
			isCompiled = false;
		}
	}

	
	if (showConsole) {
		if (ImGui::Button("Hide Console")) {
			showConsole = false;
		}
	}
	else {
		if (ImGui::Button("Show Console")) {
			showConsole = true;
		}
	}
	ImGui::SameLine();

	if (showEditor) {
		if (ImGui::Button("Hide Editor")) {
			showEditor = false;
		}
	}
	else {
		if (ImGui::Button("Show Editor")) {
			showEditor = true;
		}
	}
	ImGui::SameLine();

	if (showTexture) {
		if (ImGui::Button("Hide Texture")) {
			showTexture = false;
		}
	}
	else {
		if (ImGui::Button("Show Texture")) {
			showTexture = true;
		}
	}

	ImGui::Checkbox("Node Draggable", &isDraggable);
	if(ImGui::IsWindowHovered())
	ImNodes::SetNodeDraggable(id, isDraggable);

	ImGui::PopStyleVar();

	if (showEditor) {
		ImGui::BeginChild((std::string("ScrpitFloatNodeEditorChild##") + std::to_string(id)).c_str(), ImVec2(300, 200));
		editor->Render((std::string("ScrpitFloatNodeEditor##") + std::to_string(id)).c_str());
		ImGui::EndChild();
	}

	if (showConsole) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
		if (ImGui::Button("Clear")) {
			output.clear();
		}
		ImGui::PopStyleVar();
		ImGui::BeginChild((std::string("ScrpitFloatNodeConsoleChild##") + std::to_string(id)).c_str(), ImVec2(300, 100));
		for (int i = 0; i < output.size(); i++) {
			ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			if (output[i].first == 1)
				color = ImVec4(0.7f, 0.2f, 0.2f, 1.0f);
			if (output[i].first == 2)
				color = ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
			ImGui::TextColored(color, output[i].second.c_str());
		}
		ImGui::EndChild();
	}

	if (showEditor && outputTex) {
		ImGui::Image((ImTextureID)outputTex->GetRendererID(), ImVec2(200, 200));
	}


	ImNodes::EndNode();
	return true;
}


ScriptFloatNode::~ScriptFloatNode(){
	lua_close(L);
	delete editor;
}

// -------SCRIPT-FLOAT--NODE-END----------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------
