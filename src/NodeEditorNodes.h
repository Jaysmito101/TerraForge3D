#pragma once
#include <string>
#include <iostream>
#include <imgui.h>
#include <imnodes/imnodes.h>
#include <vector>
#include <functional>
#include <map>
#include <algorithm>
#include <ViewportFramebuffer.h>
#include <json.hpp>
#include <text-editor/TextEditor.h>
#include <Texture2D.h>
#include <lua-5.4.3/src/lua.hpp>
#include <lua-5.4.3/src/lualib.h>
#include <lua-5.4.3/src/lauxlib.h>

#define STR(x) std::to_string(x)


int GenerateId();

enum NodeType
{
	FloatNodeI = 0,
	FloatNodeO,
	Add,
	Sub,
	Mul,
	Div,
	Sin,
	Cos,
	MeshCoord,
	Square,
	Sqrt,
	Abs,
	Mix,
	Clamp,
	ScriptF,
	TextureSampler2D,
	Random,
	Perlin,
	Cellular,
	Generator,
	Time,
	Condition,
	Duplicate
};

struct NodeData {
	NodeData(){}

	NodeData(int* res)
		:resolution(res) {}


	int *resolution;
};

struct Link
{
	int id;
	int start_attr, end_attr;
	void* other;

	virtual nlohmann::json Save() {
		nlohmann::json data;
		data["id"] = id;
		data["start_attr"] = start_attr;
		data["end_attr"] = end_attr;
		return data;
	};
	virtual void Load(nlohmann::json data) {
		id = data["id"];
		start_attr = data["start_attr"];
		end_attr = data["end_attr"];
	};

	bool operator==(const Link& a) const
	{
		return (id == a.id);
	}
};


class Node
{
public:
	Node(std::string nodeName, int nodeId = GenerateId())
		:name(nodeName), id(nodeId) {}

	virtual bool Render() = 0;
	virtual void Setup() {};
	virtual float EvaluatePin(float x, float y, int id) = 0;
	virtual std::vector<void*>  GetPins() = 0;

	virtual nlohmann::json Save() = 0;
	virtual void Load(nlohmann::json data) = 0;

	int id = -1;
	std::string name;
	NodeData data;
};

enum PinType {
	Input = 0,
	Output = 1
};




class Pin
{
public:

	Pin(Node* parentNode, PinType pinType = PinType::Input, int pinId = GenerateId()) {
		id = pinId;
		node = parentNode;
		type = pinType;
		isLinked = false;
	}

	virtual nlohmann::json Save() {
		nlohmann::json data;
		data["link"] = link.Save();
		data["isLinked"] = isLinked;
		data["type"] = type;
		data["id"] = id;
		return data;
	};

	virtual void Load(nlohmann::json data) {
		link.Load(data["link"]);
		isLinked = data["isLinked"];
		type = data["type"];
		id = data["id"];
	};

	virtual bool AllowConnection(PinType t) {
		return true;
	}

	int id = -1;
	Link link;
	Node* node;
	bool isLinked;
	PinType type;
};

struct Editor
{
	ImNodesEditorContext* context = nullptr;
	std::vector<Node*>     nodes;
	std::vector<Link>     links;
	std::vector<Pin*>     pins;
	int                   current_id = 0;

	nlohmann::json Save(Node* outputNode) {
		nlohmann::json data;
		data["context"] = ImNodes::SaveEditorStateToIniString(context);
		data["type"] = "EDITOR";
		std::vector<nlohmann::json> nodesPositionSave;
		std::vector<nlohmann::json> nodesSave;
		ImVec2 pos;
		for (Node* n : nodes) {
			nodesSave.push_back(n->Save());
			pos = ImNodes::GetNodeEditorSpacePos(n->id);
			nodesPositionSave.push_back(nlohmann::json({ {"x", pos.x}, {"y", pos.y}, {"id", n->id} }));
		}

		//nodesPositionSave.push_back(nlohmann::json({ {"x", pos.x}, {"y", pos.y}, {"id", outputNode->id} }));
		data["nodes"] = nodesSave;
		data["nodePositions"] = nodesPositionSave;
		data["outputNode"] = outputNode->Save().dump();
		std::vector<nlohmann::json> linksSave;
		for (Link& l : links) {
			linksSave.push_back(l.Save());
		}
		data["links"] = linksSave;



		return data;
	}
	void Load(nlohmann::json data) {

	}

	Pin* FindPin(int id) {
		for (Pin* p : pins) {
			if (p)
				if (p->id == id)
					return p;
		}
		return nullptr;
	}
};

class FloatPin : public Pin
{
public:

	FloatPin(Node* parentNode, PinType pinType = PinType::Input, int pinId = GenerateId()) 
		:Pin(parentNode, pinType, pinId)
	{
	}

	float Evaluate(float x, float y) {
		if (type == PinType::Output) {
			{
				return node->EvaluatePin(x, y, id);
			}
		}
		else if (type == PinType::Input) {
			if (isLinked) {
				FloatPin* otherPin = (FloatPin*)link.other;
				if (otherPin) {
					return otherPin->Evaluate(x, y);
				}
			}
			else {
				return 0.0f;
			}
		}
		return 0.0f;

	}

};



class FloatValueI : public Node {
public:
	FloatValueI(std::string name = "Float Value", int nodeId = GenerateId())
		: Node(name, nodeId) {}

	virtual float EvaluatePin(float x, float y, int id) override {
		return value;
	}

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::FloatNodeI;
		data["inputPin"] = inputPin.Save();
		data["value"] = value;
		data["oID"] = oiID;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPin.Load(data["inputPin"]);
		inputPin.node = this;
		value = data["value"];
		oiID = data["oID"];
		id = data["id"];
		name = data["name"];
	}

	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPin });
	};

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		if (!inputPin.isLinked) 
		{
			ImNodes::BeginInputAttribute(inputPin.id);
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Value##floatValueNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
			ImGui::PopItemWidth();
			ImNodes::EndInputAttribute();
		}
		else {
			ImNodes::BeginInputAttribute(inputPin.id);
			// Linked Node uses input value
			ImNodes::EndInputAttribute();
		}

			ImGui::Dummy(ImVec2(100, 110));
			ImGui::Image((ImTextureID)outputTex, ImVec2(100, -100));
		


		ImNodes::EndNode();


		return true;
	}

	virtual float Evaluate(float x, float y)  {
		if (inputPin.isLinked) {
			return inputPin.Evaluate(x, y);
		}
		return value;
	}

	uint32_t outputTex;
	FloatPin inputPin = FloatPin(this);
	float value = 0.0f;
private:
	int oiID = GenerateId();
};

class FloatValueO : public Node {
public:
	FloatValueO(std::string name = "Float Value", int id = GenerateId());

	virtual void Setup() override;


	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);


	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};

class SinNode : public Node {
public:
	SinNode(std::string name = "Sin", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		outputPin.node = this;
		inputPin.node = this;
	}


	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPin, &outputPin });
	};

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::Sin;
		data["inputPin"] = inputPin.Save();
		data["outputPin"] = outputPin.Save();
		data["value"] = value;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPin.Load(data["inputPin"]);
		outputPin.Load(data["outputPin"]);
		value = data["value"];
		id = data["id"];
		name = data["name"];
	}

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		if (!inputPin.isLinked)
		{
			ImNodes::BeginInputAttribute(inputPin.id);
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Value##floatValueNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
			ImGui::PopItemWidth();
			ImNodes::EndInputAttribute();
		}
		else {
			ImNodes::BeginInputAttribute(inputPin.id);
			ImGui::Dummy(ImVec2(100, 20));
			ImNodes::EndInputAttribute();
		}

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImNodes::EndOutputAttribute();


		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		if (inputPin.isLinked) {
			return sin(inputPin.Evaluate(x, y));
		}
		return sin(value);
	}

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};

class CosNode : public Node {
public:
	CosNode(std::string name = "Cos", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		outputPin.node = this;
		inputPin.node = this;
	}


	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPin, &outputPin });
	};

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::Cos;
		data["inputPin"] = inputPin.Save();
		data["outputPin"] = outputPin.Save();
		data["value"] = value;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPin.Load(data["inputPin"]);
		outputPin.Load(data["outputPin"]);
		value = data["value"];
		id = data["id"];
		name = data["name"];
	}

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		if (!inputPin.isLinked)
		{
			ImNodes::BeginInputAttribute(inputPin.id);
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Value##floatValueNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
			ImGui::PopItemWidth();
			ImNodes::EndInputAttribute();
		}
		else {
			ImNodes::BeginInputAttribute(inputPin.id);
			ImGui::Dummy(ImVec2(100, 20));
			ImNodes::EndInputAttribute();
		}

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImNodes::EndOutputAttribute();


		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		if (inputPin.isLinked) {
			return cos(inputPin.Evaluate(x, y));
		}
		return cos(value);
	}

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};

class AdderNode : public Node {
public:
	AdderNode(std::string name = "Add", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		inputPinX.node = this;
		inputPinY.node = this;
		outputPin.node = this;
	}


	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPinX, &inputPinY, &outputPin });
	};

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::Add;
		data["inputPinX"] = inputPinX.Save();
		data["inputPinY"] = inputPinY.Save();
		data["outputPin"] = outputPin.Save();
		data["xv"] = xv;
		data["yv"] = yv;
		data["useXY"] = useXY;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPinX.Load(data["inputPinX"]);
		inputPinY.Load(data["inputPinY"]);
		outputPin.Load(data["outputPin"]);
		xv = data["xv"];
		yv = data["yv"];
		useXY = data["useXY"];
		id = data["id"];
		name = data["name"];
		Setup();
	}

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginStaticAttribute(oID);
		ImGui::Checkbox((std::string("Use Mesh XY##") + STR(oID)).c_str(), &useXY);
		ImNodes::EndStaticAttribute();

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImGui::Text("Output");
		ImNodes::EndOutputAttribute();

		ImNodes::BeginInputAttribute(inputPinX.id);
		if (inputPinX.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("X##floatValueNode") + std::to_string(inputPinX.id)).c_str(), &xv, 0.01f);
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inputPinY.id);
		if (inputPinY.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Y##floatValueNode") + std::to_string(inputPinY.id)).c_str(), &yv, 0.01f);
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		float xo = 0;
		float yo = 0;
		if (useXY) {
			xv = x;
			yv = y;
		}
		if (inputPinX.isLinked) {
			xo = inputPinX.Evaluate(x, y);
		}
		else {
			xo = xv;
		}
		if (inputPinY.isLinked) {
			yo = inputPinY.Evaluate(x, y);
		}
		else {
			yo = yv;
		}
		return xo + yo;
	}

	FloatPin outputPin = FloatPin(this, PinType::Output);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	float xv = 0.0f;
	float yv = 0.0f;
	bool useXY = false;
	int oID = GenerateId();
};

class SubtractNode : public Node {
public:
	SubtractNode(std::string name = "Subtract", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		inputPinX.node = this;
		inputPinY.node = this;
		outputPin.node = this;
	}

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::Sub;
		data["inputPinX"] = inputPinX.Save();
		data["inputPinY"] = inputPinY.Save();
		data["outputPin"] = outputPin.Save();
		data["xv"] = xv;
		data["yv"] = yv;
		data["useXY"] = useXY;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPinX.Load(data["inputPinX"]);
		inputPinY.Load(data["inputPinY"]);
		outputPin.Load(data["outputPin"]);
		xv = data["xv"];
		yv = data["yv"];
		useXY = data["useXY"];
		id = data["id"];
		name = data["name"];
		Setup();
	}

	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPinX, &inputPinY, &outputPin });
	};

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginStaticAttribute(oID);
		ImGui::Checkbox((std::string("Use Mesh XY##") + STR(oID)).c_str(), &useXY);
		ImNodes::EndStaticAttribute();

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImGui::Text("Output");
		ImNodes::EndOutputAttribute();

		ImNodes::BeginInputAttribute(inputPinX.id);
		if (inputPinX.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("X##floatValueNode") + std::to_string(inputPinX.id)).c_str(), &xv, 0.01f);
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inputPinY.id);
		if (inputPinY.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Y##floatValueNode") + std::to_string(inputPinY.id)).c_str(), &yv, 0.01f);
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		float xo = 0;
		float yo = 0;
		if (useXY) {
			xv = x;
			yv = y;
		}
		if (inputPinX.isLinked) {
			xo = inputPinX.Evaluate(x, y);
		}
		else {
			xo = xv;
		}
		if (inputPinY.isLinked) {
			yo = inputPinY.Evaluate(x, y);
		}
		else {
			yo = yv;
		}
		return xo - yo;
	}

	FloatPin outputPin = FloatPin(this, PinType::Output);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	float xv = 0.0f;
	float yv = 0.0f;
	bool useXY = false;
	int oID = GenerateId();
};

class MultiplyNode : public Node {
public:
	MultiplyNode(std::string name = "Multiply", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		inputPinX.node = this;
		inputPinY.node = this;
		outputPin.node = this;
	}

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::Mul;
		data["inputPinX"] = inputPinX.Save();
		data["inputPinY"] = inputPinY.Save();
		data["outputPin"] = outputPin.Save();
		data["xv"] = xv;
		data["yv"] = yv;
		data["useXY"] = useXY;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPinX.Load(data["inputPinX"]);
		inputPinY.Load(data["inputPinY"]);
		outputPin.Load(data["outputPin"]);
		xv = data["xv"];
		yv = data["yv"];
		useXY = data["useXY"];
		id = data["id"];
		name = data["name"];
		Setup();
	}

	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPinX, &inputPinY, &outputPin });
	};

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginStaticAttribute(oID);
		ImGui::Checkbox((std::string("Use Mesh XY##") + STR(oID)).c_str(), &useXY);
		ImNodes::EndStaticAttribute();

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImGui::Text("Output");
		ImNodes::EndOutputAttribute();

		ImNodes::BeginInputAttribute(inputPinX.id);
		if (inputPinX.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("X##floatValueNode") + std::to_string(inputPinX.id)).c_str(), &xv, 0.01f);
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inputPinY.id);
		if (inputPinY.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Y##floatValueNode") + std::to_string(inputPinY.id)).c_str(), &yv, 0.01f);
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		float xo = 0;
		float yo = 0;
		if (useXY) {
			xv = x;
			yv = y;
		}
		if (inputPinX.isLinked) {
			xo = inputPinX.Evaluate(x, y);
		}
		else {
			xo = xv;
		}
		if (inputPinY.isLinked) {
			yo = inputPinY.Evaluate(x, y);
		}
		else {
			yo = yv;
		}
		return xo * yo;
	}

	FloatPin outputPin = FloatPin(this, PinType::Output);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	float xv = 0.0f;
	float yv = 0.0f;
	bool useXY = false;
	int oID = GenerateId();
};

class DivideNode : public Node {
public:
	DivideNode(std::string name = "Divide", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		inputPinX.node = this;
		inputPinY.node = this;
		outputPin.node = this;
	}

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::Div;
		data["inputPinX"] = inputPinX.Save();
		data["inputPinY"] = inputPinY.Save();
		data["outputPin"] = outputPin.Save();
		data["xv"] = xv;
		data["yv"] = yv;
		data["useXY"] = useXY;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPinX.Load(data["inputPinX"]);
		inputPinY.Load(data["inputPinY"]);
		outputPin.Load(data["outputPin"]);
		xv = data["xv"];
		yv = data["yv"];
		useXY = data["useXY"];
		id = data["id"];
		name = data["name"];
		Setup();
	}

	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPinX, &inputPinY, &outputPin });
	};

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginStaticAttribute(oID);
		ImGui::Checkbox((std::string("Use Mesh XY##") + STR(oID)).c_str(), &useXY);
		ImNodes::EndStaticAttribute();

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImGui::Text("Output");
		ImNodes::EndOutputAttribute();

		ImNodes::BeginInputAttribute(inputPinX.id);
		if (inputPinX.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("X##floatValueNode") + std::to_string(inputPinX.id)).c_str(), &xv, 0.01f);
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::BeginInputAttribute(inputPinY.id);
		if (inputPinY.isLinked || useXY) {
			ImGui::Dummy(ImVec2(100, 20));
		}
		else {
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Y##floatValueNode") + std::to_string(inputPinY.id)).c_str(), &yv, 0.01f);
			if (yv == 0) {
				ImGui::TextUnformatted("Since value is 0 it will be converted to 1.");
			}
			ImGui::PopItemWidth();
		}
		ImNodes::EndInputAttribute();

		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		float xo = 0;
		float yo = 0;
		if (useXY) {
			xv = x;
			yv = y;
		}
		if (inputPinX.isLinked) {
			xo = inputPinX.Evaluate(x, y);
		}
		else {
			xo = xv;
		}
		if (inputPinY.isLinked) {
			yo = inputPinY.Evaluate(x, y);
		}
		else {
			yo = yv;
		}
		if (yo == 0) {
			yo = 1;
		}
		return xo / yo;
	}

	FloatPin outputPin = FloatPin(this, PinType::Output);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	float xv = 0.0f;
	float yv = 0.0f;
	bool useXY = false;
	int oID = GenerateId();
};

class MeshCoordNode : public Node {
public:
	MeshCoordNode(std::string name = "Mesh Coordinates", int id = GenerateId());

	virtual void Setup() override;

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual std::vector<void*>  GetPins();

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin outputPinX = FloatPin(this, PinType::Output);
	FloatPin outputPinY = FloatPin(this, PinType::Output);
	float value = 0.0f;

};

class SquareNode : public Node {
public:
	SquareNode(std::string name = "Square", int id = GenerateId());

	virtual void Setup() override;


	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};

class SqrtNode : public Node {
public:
	SqrtNode(std::string name = "Square Root", int id = GenerateId());

	virtual void Setup() override;


	virtual std::vector<void*>  GetPins();
	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};

class AbsNode : public Node {
public:
	AbsNode(std::string name = "Absolute Value", int id = GenerateId());

	virtual void Setup() override;


	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};


class ClampNode : public Node {
public:
	ClampNode(std::string name = "Clamp", int id = GenerateId());

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.5f;
	float min = 0.0f;
	float max = 1.0f;

};

class ScriptFloatNode : public Node {
public:
	ScriptFloatNode(std::string name = "Script", int id = GenerateId())
		:Node(name, id) {};

	~ScriptFloatNode();

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float a = 0;
	float b = 0;
	float c = 0;
private:
	lua_State* L;
	TextEditor* editor;
	Texture2D* outputTex;
	std::vector<std::pair<int, std::string>> output;
	bool isCompiled = false;
	bool isDraggable = true;
	bool showConsole = false;
	bool showEditor = true;
	bool showTexture = false;
};


class MixNode : public Node {
public:
	MixNode(std::string name = "Mix", int id = GenerateId());

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float a = 0.0f;
	float b = 1.0f;
	float factor = 0.5f;

};


class TextureSamplerNode : public Node {
public:

	TextureSamplerNode(std::string name = "Texture Sampler", int id = GenerateId())
		: Node(name, id) {}

	virtual void Setup() override;


	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);


	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

private:
	Texture2D* texture;
	std::string textureFilePath;
	FloatPin outputPinR = FloatPin(this, PinType::Output);
	FloatPin outputPinG = FloatPin(this, PinType::Output);
	FloatPin outputPinB = FloatPin(this, PinType::Output);
	bool isLoaded = false;

};




class RandomNode : public Node {
public:
	RandomNode(std::string name = "Random", int id = GenerateId());

	virtual void Setup() override;


	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	int value = 0.0f;
	int max = 10;
	int min = 0;

};



class TimeNode : public Node {
public:
	TimeNode(std::string name = "Time", int id = GenerateId());

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);


	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin outputPin = FloatPin(this, PinType::Output);
};



class PerlinNode : public Node {
public:
	PerlinNode(std::string name = "Simplex Noise", int id = GenerateId());

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPinT = FloatPin(this, PinType::Input);
	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float a = 1.0f;
	float b = 0.0f;
	float factor = 0.0f;
	float strength = 1.0f;
};



class CellularNode : public Node {
public:
	CellularNode(std::string name = "Cellular Noise", int id = GenerateId());

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPinT = FloatPin(this, PinType::Input);
	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float a = 1.0f;
	float b = 0.0f;
	float factor = 0.0f;
	float strength = 1.0f;
};

class GeneratorNode : public Node {
public:
	GeneratorNode(std::string name = "Generator", int id = GenerateId());

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPinT = FloatPin(this, PinType::Input);
	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	int gridSize = 4;
	int gridSizeOld = 4;
	bool *grid;
	float smoothness = 1.0f;
};

class ConditionNode : public Node {
public:
	ConditionNode(std::string name = "Condition", int id = GenerateId());

	virtual void Setup() override;

	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);

	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float a = 0.0f;
	float b = 1.0f;
	float factor = 0.5f;
	float condValue;
};

class DuplicateNode : public Node {
public:

	DuplicateNode(std::string name = "Duplicate", int id = GenerateId());

	virtual void Setup() override;


	virtual std::vector<void*>  GetPins();

	nlohmann::json Save();

	void Load(nlohmann::json data);


	virtual bool Render() override;

	virtual float EvaluatePin(float x, float y, int id) override;

	FloatPin inputPin = FloatPin(this, PinType::Input);
private:
	FloatPin outputPinX = FloatPin(this, PinType::Output);
	FloatPin outputPinY = FloatPin(this, PinType::Output);
	FloatPin outputPinZ = FloatPin(this, PinType::Output);
};

