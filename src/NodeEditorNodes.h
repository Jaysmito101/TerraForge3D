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
	FloatNode = 0,
	Vec3Node
};

struct Link
{
	int id;
	int start_attr, end_attr;
	void* other;
};


class Node
{
public:
	Node(std::string nodeName, int nodeId = GenerateId())
		:name(nodeName), id(nodeId) {}

	virtual bool Render() = 0;
	virtual void Setup() {};
	virtual float EvaluatePin(float x, float y, int id) = 0;

	NodeType type;
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
			ImGui::Image((ImTextureID)GetViewportFramebufferColorTextureId(), ImVec2(100, -100));
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

	uint32_t outputImage = -1;
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
