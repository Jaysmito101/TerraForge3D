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

#define STR(x) std::to_string(x)

static int GenerateId() {
	static int id = 1;
	int retVal = id;
	id = id + 1;
	return retVal;
}

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
	Midval,
	Mix,
	Clamp
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
		pos = ImNodes::GetNodeEditorSpacePos(outputNode->id);
		nodesPositionSave.push_back(nlohmann::json({ {"x", pos.x}, {"y", pos.y}, {"id", outputNode->id} }));
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

		if (true) {
			ImNodes::BeginStaticAttribute(oiID);
			ImGui::Dummy(ImVec2(100, 110));
			ImGui::Image((ImTextureID)outputTex, ImVec2(100, -100));
			ImNodes::EndStaticAttribute();
		}


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
	FloatValueO(std::string name = "Float Value", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		outputPin.node = this;
	}


	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &outputPin });
	};

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::FloatNodeO;
		data["outputPin"] = outputPin.Save();
		data["value"] = value;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
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

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat((std::string("Value##floatValueNode") + std::to_string(outputPin.id)).c_str(), &value, 0.01f);
		ImGui::PopItemWidth();
		ImNodes::EndOutputAttribute();


		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		return value;
	}

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
	MeshCoordNode(std::string name = "Mesh Coordinates", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		outputPinX.node = this;
		outputPinY.node = this;
	}

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::MeshCoord;
		data["outputPinX"] = outputPinX.Save();
		data["outputPinY"] = outputPinY.Save();
		data["value"] = value;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		outputPinX.Load(data["outputPinX"]);
		outputPinY.Load(data["outputPinY"]);
		value = data["value"];
		id = data["id"];
		name = data["name"];
		Setup();
	}

	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &outputPinX, &outputPinX });
	};

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginOutputAttribute(outputPinX.id);
		ImGui::Text("X");
		ImNodes::EndOutputAttribute();

		ImNodes::BeginOutputAttribute(outputPinY.id);
		ImGui::Text("Y");
		ImNodes::EndOutputAttribute();

		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		if (id == outputPinX.id)
			return x;
		if (id == outputPinY.id)
			return y;
	}

	FloatPin outputPinX = FloatPin(this, PinType::Output);
	FloatPin outputPinY = FloatPin(this, PinType::Output);
	float value = 0.0f;

};


class SquareNode : public Node {
public:
	SquareNode(std::string name = "Square", int id = GenerateId())
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
		data["type"] = NodeType::Square;
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
			ImGui::DragFloat((std::string("Value##squareNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
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
			return (inputPin.Evaluate(x, y)) * (inputPin.Evaluate(x, y));
		}
		return (value * value);
	}

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};


class SqrtNode : public Node {
public:
	SqrtNode(std::string name = "Square Root", int id = GenerateId())
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
		data["type"] = NodeType::Sqrt;
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
			ImGui::DragFloat((std::string("Value##sqrtNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
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
			return sqrt(inputPin.Evaluate(x, y));
		}
		return sqrt(value);
	}

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};



class AbsNode : public Node {
public:
	AbsNode(std::string name = "Absolute Value", int id = GenerateId())
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
		data["type"] = NodeType::Abs;
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
			ImGui::DragFloat((std::string("Value##sqrtNode") + std::to_string(inputPin.id)).c_str(), &value, 0.01f);
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
			return abs(inputPin.Evaluate(x, y));
		}
		return abs(value);
	}

	FloatPin inputPin = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.0f;

};


class ClampNode : public Node {
public:
	ClampNode(std::string name = "Clamp", int id = GenerateId())
		: Node(name, id) {
	}

	virtual void Setup() override {
		outputPin.node = this;
		inputPinV.node = this;
		inputPinX.node = this;
		inputPinY.node = this;
	}


	virtual std::vector<void*>  GetPins() {
		return std::vector<void*>({ &inputPinX, &inputPinY, &inputPinV, &outputPin });
	};

	nlohmann::json Save() {
		nlohmann::json data;
		data["type"] = NodeType::Clamp;
		data["inputPinZ"] = inputPinV.Save();
		data["inputPinX"] = inputPinX.Save();
		data["inputPinY"] = inputPinY.Save();
		data["outputPin"] = outputPin.Save();
		data["value"] = value;
		data["min"] = min;
		data["max"] = max;
		data["id"] = id;
		data["name"] = name;
		return data;
	}
	void Load(nlohmann::json data) {
		inputPinV.Load(data["inputPinV"]);
		inputPinX.Load(data["inputPinX"]);
		inputPinY.Load(data["inputPinY"]);
		outputPin.Load(data["outputPin"]);
		value = data["value"];
		min = data["min"];
		max = data["max"];
		id = data["id"];
		name = data["name"];
	}

	virtual bool Render() override {
		ImNodes::BeginNode(id);

		ImNodes::BeginNodeTitleBar();
		ImGui::TextUnformatted((name).c_str());
		ImNodes::EndNodeTitleBar();

		ImNodes::BeginOutputAttribute(outputPin.id);
		ImNodes::EndOutputAttribute();

		if (!inputPinV.isLinked)
		{
			ImNodes::BeginInputAttribute(inputPinV.id);
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Value##clampNode") + std::to_string(inputPinV.id)).c_str(), &value, 0.01f);
			ImGui::PopItemWidth();
			ImNodes::EndInputAttribute();
		}
		else {
			ImNodes::BeginInputAttribute(inputPinV.id);
			ImGui::Dummy(ImVec2(100, 20));
			ImNodes::EndInputAttribute();
		}

		if (!inputPinX.isLinked)
		{
			ImNodes::BeginInputAttribute(inputPinX.id);
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Value##clampNode") + std::to_string(inputPinX.id)).c_str(), &min, 0.01f);
			ImGui::PopItemWidth();
			ImNodes::EndInputAttribute();
		}
		else {
			ImNodes::BeginInputAttribute(inputPinX.id);
			ImGui::Dummy(ImVec2(100, 20));
			ImNodes::EndInputAttribute();
		}

		if (!inputPinY.isLinked)
		{
			ImNodes::BeginInputAttribute(inputPinY.id);
			ImGui::PushItemWidth(100);
			ImGui::DragFloat((std::string("Value##clampNode") + std::to_string(inputPinY.id)).c_str(), &max, 0.01f);
			ImGui::PopItemWidth();
			ImNodes::EndInputAttribute();
		}
		else {
			ImNodes::BeginInputAttribute(inputPinY.id);
			ImGui::Dummy(ImVec2(100, 20));
			ImNodes::EndInputAttribute();
		}


		ImNodes::EndNode();
		return true;
	}

	virtual float EvaluatePin(float x, float y, int id) override {
		float minV = min;
		float maxV = max;
		float valV = value;

		if (inputPinV.isLinked) {
			valV = (inputPinV.Evaluate(x, y));
		}
		if (inputPinX.isLinked) {
			minV = (inputPinX.Evaluate(x, y));
		}
		if (inputPinY.isLinked) {
			maxV = (inputPinY.Evaluate(x, y));
		}
		if (valV > maxV)
			return max;
		if (valV < minV)
			return min;
		return valV;

	}

	FloatPin inputPinV = FloatPin(this, PinType::Input);
	FloatPin inputPinX = FloatPin(this, PinType::Input);
	FloatPin inputPinY = FloatPin(this, PinType::Input);
	FloatPin outputPin = FloatPin(this, PinType::Output);
	float value = 0.5f;
	float min = 0.0f;
	float max = 1.0f;

};