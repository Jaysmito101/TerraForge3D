#include "Misc/CustomInspector.h"
#include "Base/Base.h"
#include "Utils/Utils.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

std::string CustomInspectorValue::CustomInspectorValueTypeToString(CustomInspectorValueType type)
{
	switch (type)
	{
	case CustomInspectorValueType_Int:		return "Int";
	case CustomInspectorValueType_Float:	return "Float";
	case CustomInspectorValueType_Bool:		return "Bool";
	case CustomInspectorValueType_String:	return "String";
	case CustomInspectorValueType_Vector2:	return "Vector2";
	case CustomInspectorValueType_Vector3:	return "Vector3";
	case CustomInspectorValueType_Vector4:	return "Vector4";
	case CustomInspectorValueType_Texture:	return "Texture";
	default: return "Unknown";
	}
}

CustomInspectorValueType CustomInspectorValue::CustomInspectorValueTypeFromString(const std::string& type)
{
	if (type == "Int")		return CustomInspectorValueType_Int;
	if (type == "Float")	return CustomInspectorValueType_Float;
	if (type == "Bool")		return CustomInspectorValueType_Bool;
	if (type == "String")	return CustomInspectorValueType_String;
	if (type == "Vector2")	return CustomInspectorValueType_Vector2;
	if (type == "Vector3")	return CustomInspectorValueType_Vector3;
	if (type == "Vector4")	return CustomInspectorValueType_Vector4;
	if (type == "Texture")	return CustomInspectorValueType_Texture;
	return CustomInspectorValueType_Unknown;
}

SerializerNode CustomInspectorValue::Save() const
{
	SerializerNode node = CreateSerializerNode();
	node->SetString("Name", m_Name);
	node->SetInteger("Type", static_cast<int32_t>(m_Type));
	node->SetString("TypeName", CustomInspectorValueTypeToString(m_Type));
	switch (m_Type)
	{
	case CustomInspectorValueType_Int:
		node->SetInteger("Value", m_IntValue);
		node->SetInteger("DefaultValue", m_DefaultIntValue);
		break;
	case CustomInspectorValueType_Float:
		node->SetFloat("Value", m_FloatValue);
		node->SetFloat("DefaultValue", m_DefaultFloatValue);
		break;
	case CustomInspectorValueType_Bool:
		node->SetString("Value", m_BoolValue ? "true" : "false");
		node->SetString("DefaultValue", m_DefaultBoolValue ? "true" : "false");
		break;
	case CustomInspectorValueType_String:
		node->SetString("Value", m_StringValue);
		node->SetString("DefaultValue", m_DefaultStringValue);
		break;
	case CustomInspectorValueType_Vector2:
		node->SetFloat("ValueX", m_VectorValue[0]);
		node->SetFloat("ValueY", m_VectorValue[1]);
		node->SetFloat("DefaultValueX", m_DefaultVectorValue[0]);
		node->SetFloat("DefaultValueY", m_DefaultVectorValue[1]);
		break;
	case CustomInspectorValueType_Vector3:
		node->SetFloat("ValueX", m_VectorValue[0]);
		node->SetFloat("ValueY", m_VectorValue[1]);
		node->SetFloat("ValueZ", m_VectorValue[2]);
		node->SetFloat("DefaultValueX", m_DefaultVectorValue[0]);
		node->SetFloat("DefaultValueY", m_DefaultVectorValue[1]);
		node->SetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
		break;
	case CustomInspectorValueType_Vector4:
		node->SetFloat("ValueX", m_VectorValue[0]);
		node->SetFloat("ValueY", m_VectorValue[1]);
		node->SetFloat("ValueZ", m_VectorValue[2]);
		node->SetFloat("ValueW", m_VectorValue[3]);
		node->SetFloat("DefaultValueX", m_DefaultVectorValue[0]);
		node->SetFloat("DefaultValueY", m_DefaultVectorValue[1]);
		node->SetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
		node->SetFloat("DefaultValueW", m_DefaultVectorValue[3]);
		break;
	case CustomInspectorValueType_Texture:
		node->SetFile("Value", m_TextureValue ? m_TextureValue->GetPath() : "null");
		node->SetFile("DefaultValue", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "null");
		break;
	}
	return node;
}

void CustomInspectorValue::Load(const SerializerNode& node)
{
	m_Type = CustomInspectorValueTypeFromString(node->GetString("TypeName", CustomInspectorValueTypeToString(m_Type)));
	m_Name = node->GetString("Name");
	switch (m_Type)
	{
	case CustomInspectorValueType_Int:
		m_DefaultIntValue = node->GetInteger("DefaultValue", m_DefaultIntValue);
		m_IntValue = node->GetInteger("Value", m_DefaultIntValue);
		break;
	case CustomInspectorValueType_Float:
		m_DefaultFloatValue = node->GetFloat("DefaultValue", m_DefaultFloatValue);
		m_FloatValue = node->GetFloat("Value", m_DefaultFloatValue);
		break;
	case CustomInspectorValueType_Bool:
		m_DefaultBoolValue = node->GetString("DefaultValue", m_DefaultBoolValue ? "true" : "false") == "true";
		m_BoolValue = node->GetString("Value", m_DefaultBoolValue ? "true" : "false") == "true";
		break;
	case CustomInspectorValueType_String:
		m_DefaultStringValue = node->GetString("DefaultValue", m_DefaultStringValue);
		m_StringValue = node->GetString("Value", m_DefaultStringValue);
		break;
	case CustomInspectorValueType_Vector2:
		m_DefaultVectorValue[0] = node->GetFloat("DefaultValueX", m_DefaultVectorValue[0]);
		m_DefaultVectorValue[1] = node->GetFloat("DefaultValueY", m_DefaultVectorValue[1]);
		m_VectorValue[0] = node->GetFloat("ValueX", m_DefaultVectorValue[0]);
		m_VectorValue[1] = node->GetFloat("ValueY", m_DefaultVectorValue[1]);
		break;
	case CustomInspectorValueType_Vector3:
		m_DefaultVectorValue[0] = node->GetFloat("DefaultValueX", m_DefaultVectorValue[0]);
		m_DefaultVectorValue[1] = node->GetFloat("DefaultValueY", m_DefaultVectorValue[1]);
		m_DefaultVectorValue[2] = node->GetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
		m_VectorValue[0] = node->GetFloat("ValueX", m_DefaultVectorValue[0]);
		m_VectorValue[1] = node->GetFloat("ValueY", m_DefaultVectorValue[1]);
		m_VectorValue[2] = node->GetFloat("ValueZ", m_DefaultVectorValue[2]);
		break;
	case CustomInspectorValueType_Vector4:
		m_DefaultVectorValue[0] = node->GetFloat("DefaultValueX", m_DefaultVectorValue[0]);
		m_DefaultVectorValue[1] = node->GetFloat("DefaultValueY", m_DefaultVectorValue[1]);
		m_DefaultVectorValue[2] = node->GetFloat("DefaultValueZ", m_DefaultVectorValue[2]);
		m_DefaultVectorValue[3] = node->GetFloat("DefaultValueW", m_DefaultVectorValue[3]);
		m_VectorValue[0] = node->GetFloat("ValueX", m_DefaultVectorValue[0]);
		m_VectorValue[1] = node->GetFloat("ValueY", m_DefaultVectorValue[1]);
		m_VectorValue[2] = node->GetFloat("ValueZ", m_DefaultVectorValue[2]);
		m_VectorValue[3] = node->GetFloat("ValueW", m_DefaultVectorValue[3]);
		break;
	case CustomInspectorValueType_Texture:
		const auto& defaultPath = node->GetFile("DefaultValue", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "");
		const auto& path = node->GetFile("Value", m_DefaultTextureValue ? m_DefaultTextureValue->GetPath() : "");
		// delete m_DefaultTextureValue; delete m_TextureValue;
		m_DefaultTextureValue = std::make_shared<Texture2D>(defaultPath);
		m_TextureValue = std::make_shared<Texture2D>(path);
		break;
	}
}

CustomInspectorWidget::CustomInspectorWidget(CustomInspectorWidgetType type)
{
	m_Type = type;
	m_ID = GenerateId(16);
}

CustomInspectorWidget::~CustomInspectorWidget()
{
}

std::string CustomInspectorWidget::CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type)
{
	switch (type)
	{
	case CustomInspectorWidgetType_Slider:		return "Slider";
	case CustomInspectorWidgetType_Drag:		return "Drag";
	case CustomInspectorWidgetType_Color:		return "Color";
	case CustomInspectorWidgetType_Texture:		return "Texture";
	case CustomInspectorWidgetType_Button:		return "Button";
	case CustomInspectorWidgetType_Checkbox:	return "Checkbox";
	case CustomInspectorWidgetType_Input:		return "Input";
	case CustomInspectorWidgetType_Seed:		return "Seed";
	case CustomInspectorWidgetType_Dropdown:	return "Dropdown";
	case CustomInspectorWidgetType_Seperator:	return "Seperator";
	case CustomInspectorWidgetType_NewLine:		return "NewLine";
	case CustomInspectorWidgetType_Unknown:
	default: return "Unknown";
	}
}

CustomInspectorWidgetType CustomInspectorWidget::CustomInspectorWidgetTypeFromString(const std::string& type)
{
	if (type == "Slider")		return CustomInspectorWidgetType_Slider;
	if (type == "Drag")			return CustomInspectorWidgetType_Drag;
	if (type == "Color")		return CustomInspectorWidgetType_Color;
	if (type == "Texture")		return CustomInspectorWidgetType_Texture;
	if (type == "Button")		return CustomInspectorWidgetType_Button;
	if (type == "Checkbox")		return CustomInspectorWidgetType_Checkbox;
	if (type == "Input")		return CustomInspectorWidgetType_Input;
	if (type == "Seed")			return CustomInspectorWidgetType_Seed;
	if (type == "Dropdown")		return CustomInspectorWidgetType_Dropdown;
	if (type == "Seperator")	return CustomInspectorWidgetType_Seperator;
	if (type == "NewLine")		return CustomInspectorWidgetType_NewLine;
	return CustomInspectorWidgetType_Unknown;
}

SerializerNode CustomInspectorWidget::Save() const
{
	SerializerNode node = CreateSerializerNode();
	node->SetInteger("Type", static_cast<int32_t>(m_Type));
	node->SetString("TypeName", CustomInspectorWidgetTypeToString(m_Type));
	node->SetString("TargetVariable", m_VariableName);
	node->SetString("Label", m_Label);
	if (m_Type == CustomInspectorWidgetType_Seed) node->SetIntegerArray("SeedHistory", m_SeedHistory);
	node->SetInteger("ISpeed", m_ISpeed);
	node->SetFloat("FSeed", m_FSpeed);
	node->SetString("ID", m_ID);
	node->GetFloat("Contraints_0", m_Constratins[0]);
	node->GetFloat("Contraints_1", m_Constratins[1]);
	node->GetFloat("Contraints_2", m_Constratins[2]);
	node->GetFloat("Contraints_3", m_Constratins[3]);
	if (m_Tooltip.size() > 0) node->SetString("Tooltip", m_Tooltip);
	return node;
}

void CustomInspectorWidget::Load(SerializerNode node)
{
	m_Type = CustomInspectorWidgetTypeFromString(node->GetString("TypeName", CustomInspectorWidgetTypeToString(m_Type)));
	m_VariableName = node->GetString("TargetVariable", m_VariableName);
	m_Label = node->GetString("Label", m_Label);
	m_ID = node->GetString("ID", m_ID);
	if (m_Type == CustomInspectorWidgetType_Seed) m_SeedHistory = node->GetIntegerArray("SeedHistory", m_SeedHistory);
	m_ISpeed = node->GetInteger("ISpeed", m_ISpeed);
	m_FSpeed = node->GetFloat("FSeed", m_FSpeed);
	m_Constratins[0] = node->GetFloat("Contraints_0", m_Constratins[0]);
	m_Constratins[1] = node->GetFloat("Contraints_1", m_Constratins[1]);
	m_Constratins[2] = node->GetFloat("Contraints_2", m_Constratins[2]);
	m_Constratins[3] = node->GetFloat("Contraints_3", m_Constratins[3]);
	m_Tooltip = node->GetString("Tooltip", m_Tooltip);
}


CustomInspector::CustomInspector()
{
	m_ID = GenerateId(16);
	AddWidget("Seperator", CustomInspectorWidget(CustomInspectorWidgetType_Seperator));
	AddWidget("NewLine", CustomInspectorWidget(CustomInspectorWidgetType_NewLine));
}

CustomInspector::~CustomInspector()
{

}

CustomInspectorValue& CustomInspector::GetVariable(const std::string& name)
{
	return m_Values[name];
}

bool CustomInspector::HasVariable(const std::string& name)
{
	return m_Values.find(name) != m_Values.end();
}

void CustomInspector::RemoveVariable(const std::string& name)
{
	// Ideally this should not be called
	m_Values.erase(name);
	// Suggestion:
	// Also delete all widgets referencing this variable
}

CustomInspectorValue& CustomInspector::AddVariable(const std::string& name, const CustomInspectorValue& value)
{
	m_Values[name] = value;
	return (m_Values[name]);
}

CustomInspectorValue& CustomInspector::AddStringVariable(const std::string& name, const std::string& defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_String);
	value.m_StringValue = value.m_DefaultStringValue = defaultValue;
	return AddVariable(name, value);
}

CustomInspectorValue& CustomInspector::AddIntegerVariable(const std::string& name, int defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_Int);
	value.m_IntValue = value.m_DefaultIntValue = defaultValue;
	return AddVariable(name, value);
}

CustomInspectorValue& CustomInspector::AddFloatVariable(const std::string& name, float defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_Float);
	value.m_FloatValue = value.m_DefaultFloatValue = defaultValue;
	return AddVariable(name, value);
}

CustomInspectorValue& CustomInspector::AddBoolVariable(const std::string& name, bool defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_Bool);
	value.m_BoolValue = value.m_DefaultBoolValue = defaultValue;
	return AddVariable(name, value);
}

CustomInspectorValue& CustomInspector::AddVector2Variable(const std::string& name, glm::vec2 defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_Vector2);
	value.m_VectorValue[0] = value.m_VectorValue[0] = defaultValue.x;
	value.m_VectorValue[1] = value.m_VectorValue[1] = defaultValue.y;
	return AddVariable(name, value);
}

CustomInspectorValue& CustomInspector::AddVector3Variable(const std::string& name, glm::vec3 defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_Vector3);
	value.m_VectorValue[0] = value.m_VectorValue[0] = defaultValue.x;
	value.m_VectorValue[1] = value.m_VectorValue[1] = defaultValue.y;
	value.m_VectorValue[2] = value.m_VectorValue[2] = defaultValue.z;
	return AddVariable(name, value);
}

CustomInspectorValue& CustomInspector::AddVector4Variable(const std::string& name, glm::vec4 defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_Vector4);
	value.m_VectorValue[0] = value.m_VectorValue[0] = defaultValue.x;
	value.m_VectorValue[1] = value.m_VectorValue[1] = defaultValue.y;
	value.m_VectorValue[2] = value.m_VectorValue[2] = defaultValue.z;
	value.m_VectorValue[3] = value.m_VectorValue[3] = defaultValue.w;
	return AddVariable(name, value);
}

CustomInspectorValue& CustomInspector::AddTextureVariable(const std::string& name, std::shared_ptr<Texture2D> defaultValue)
{
	CustomInspectorValue value(CustomInspectorValueType_Texture);
	value.m_TextureValue = value.m_DefaultTextureValue = defaultValue;
	return AddVariable(name, value);
}

bool CustomInspector::HasWidget(const std::string& name)
{
	return m_Widgets.find(name) != m_Widgets.end();
}

CustomInspectorWidget& CustomInspector::GetWidget(const std::string& name)
{
	return m_Widgets[name];
}

void CustomInspector::RemoveWidget(const std::string& name)
{
	m_Widgets.erase(name);
}

CustomInspectorWidget& CustomInspector::AddWidget(const std::string& name, const CustomInspectorWidget& widget)
{
	if (name == "Seperator" || name == "NewLine") return GetWidget(name);
	m_Widgets[name] = widget;
	m_WidgetsOrder.push_back(name);
	return m_Widgets[name];
}

CustomInspectorWidget& CustomInspector::AddSliderWidget(const std::string& label, const std::string& variableName, float min, float max)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Slider);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	widget.m_Constratins[0] = min; widget.m_Constratins[1] = max;
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddDragWidget(const std::string& label, const std::string& variableName, float min, float max, float speed)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Drag);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	widget.m_Constratins[0] = min; widget.m_Constratins[1] = max;
	widget.m_FSpeed = speed;
	widget.m_ISpeed = static_cast<int32_t>(speed);
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddColorWidget(const std::string& label, const std::string& variableName)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Color);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddTextureWidget(const std::string& label, const std::string& variableName)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Texture);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddButtonWidget(const std::string& label, const std::string& actionName)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Button);
	widget.SetLabel(label);
	// widget.SetActionName(actionName); TODO
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddCheckboxWidget(const std::string& label, const std::string& variableName)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Checkbox);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddInputWidget(const std::string& label, const std::string& variableName)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Input);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddSeedWidget(const std::string& label, const std::string& variableName)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Seed);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddDropdownWidget(const std::string& label, const std::string& variableName, const std::vector<std::string>& options)
{
	CustomInspectorWidget widget(CustomInspectorWidgetType_Dropdown);
	widget.SetLabel(label);
	widget.SetVariableName(variableName);
	widget.m_DropdownOptions = options;
	return AddWidget(label, widget);
}

CustomInspectorWidget& CustomInspector::AddSeperatorWidget()
{
	return AddWidget("Seperator", CustomInspectorWidget(CustomInspectorWidgetType_Seperator));
}

CustomInspectorWidget& CustomInspector::AddNewLineWidget()
{
	return AddWidget("NewLine", CustomInspectorWidget(CustomInspectorWidgetType_NewLine));
}

SerializerNode CustomInspector::SaveData() const
{
	SerializerNode node = CreateSerializerNode();
	node->SetInteger("ValueCount", static_cast<int32_t>(m_Values.size()));
	node->CreateNodeArray("Values");
	for (const auto& it : m_Values)
	{
		auto subNode = it.second.Save();
		subNode->SetString("GName", it.first);
		node->PushToNodeArray("Values", subNode);
	}
	return node;
}

void CustomInspector::LoadData(SerializerNode node)
{
	m_Values.clear();
	int32_t valueCount = node->GetInteger("ValueCount");
	auto subNodes = node->GetNodeArray("Values");
	if (subNodes.size() != valueCount) std::cout << ("Warning: Data might be corrupted.\n");
	for (auto subNode : subNodes)
	{
		std::string name = subNode->GetString("GName");
		CustomInspectorValue value;
		value.Load(subNode);
		m_Values[name] = value;
	}
}

SerializerNode CustomInspector::Save() const
{
	SerializerNode node = CreateSerializerNode();
	node->SetString("ID", m_ID);
	node->SetChildNode("Data", SaveData());
	node->SetStringArray("WidgetsOrder", m_WidgetsOrder);
	node->SetInteger("WidgetsCount", static_cast<int32_t>(m_Widgets.size()));
	node->CreateNodeArray("Widgets");
	for (const auto& it : m_Widgets)
	{
		auto subNode = it.second.Save();
		subNode->SetString("GName", it.first);
		node->PushToNodeArray("Widgets", subNode);
	}
	return node;
}

void CustomInspector::Load(SerializerNode node)
{
	LoadData(node->GetChildNode("Data"));
	m_Widgets.clear();
	m_ID = node->GetString("ID", m_ID);
	int valueCount = node->GetInteger("WidgetsCount");
	auto subNodes = node->GetNodeArray("Widgets");
	if (subNodes.size() != valueCount) std::cout << ("Warning: Data might be corrupted.\n");
	for (auto subNode : subNodes)
	{
		std::string name = subNode->GetString("GName");
		CustomInspectorWidget widget;
		widget.Load(subNode);
		m_Widgets[name] = widget;
	}
}

CustomInspectorWidget& CustomInspector::SetWidgetConstraints(const std::string& label, float a, float b, float c, float d)
{
	auto& widget = m_Widgets[label];
	widget.m_Constratins[0] = a;
	widget.m_Constratins[1] = b;
	widget.m_Constratins[2] = c;
	widget.m_Constratins[3] = d;
	return widget;
}

CustomInspectorWidget& CustomInspector::SetWidgetTooltip(const std::string& label, const std::string& value)
{
	auto& widget = m_Widgets[label];
	widget.m_Tooltip = value;
	return widget;
}

CustomInspectorWidget& CustomInspector::SetWidgetSpeed(const std::string& label, float speed)
{
	auto& widget = m_Widgets[label];
	widget.m_FSpeed = speed;
	widget.m_ISpeed = static_cast<int32_t>(speed);
	return widget;
}

bool CustomInspector::Render()
{
	bool hasChanged = false;
	ImGui::PushID(m_ID.c_str());
	for (const auto& widgetLabel : m_WidgetsOrder)
	{
		const auto& widget = m_Widgets[widgetLabel];
		if (widget.m_Type == CustomInspectorWidgetType_Slider) hasChanged = RenderSlider(widget);
		else if (widget.m_Type == CustomInspectorWidgetType_Drag) hasChanged = RenderDrag(widget);
		// else if (widget.m_Type == CustomInspectorWidgetType_Color)
		// else if (widget.m_Type == CustomInspectorWidgetType_Texture) 
		// else if (widget.m_Type == CustomInspectorWidgetType_Button)
		// else if (widget.m_Type == CustomInspectorWidgetType_Checkbox)
		// else if (widget.m_Type == CustomInspectorWidgetType_Input)
		// else if (widget.m_Type == CustomInspectorWidgetType_Seed)
		else if (widget.m_Type == CustomInspectorWidgetType_Dropdown) hasChanged = RenderDropdown(widget);
		else if (widget.m_Type == CustomInspectorWidgetType_Seperator) ImGui::Separator();
		else if (widget.m_Type == CustomInspectorWidgetType_NewLine) ImGui::NewLine();
	}
	if (ImGui::Button("Reset to Defaults"))
	{
		for (auto& it : m_Values) it.second.ResetValue();
		hasChanged = true;
	}
	ImGui::PopID();
	return hasChanged;
}

bool CustomInspector::RenderSlider(const CustomInspectorWidget& widget)
{
	bool hasChanged = false;
	auto& value = m_Values[widget.m_VariableName];
	ImGui::PushID(widget.m_ID.c_str());
	switch (value.GetType())
	{
	case CustomInspectorValueType_Int: 
		hasChanged = ImGui::SliderInt(widget.m_Label.c_str(), &value.m_IntValue, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
		break;
	case CustomInspectorValueType_Float:
		hasChanged = ImGui::SliderFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_Vector2:
		hasChanged = ImGui::SliderFloat2(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_Vector3:
		hasChanged = ImGui::SliderFloat3(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_Vector4:
		hasChanged = ImGui::SliderFloat4(widget.m_Label.c_str(), value.m_VectorValue, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_String:
	case CustomInspectorValueType_Bool:
	case CustomInspectorValueType_Texture:
		throw std::exception("Invalid data type for Slider");
	}
	if (widget.m_Tooltip.size() > 0 && ImGui::IsItemHovered()) ImGui::SetTooltip(widget.m_Tooltip.c_str());
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::Button("Reset Value")) value.ResetValue();
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return hasChanged;
}

bool CustomInspector::RenderDrag(const CustomInspectorWidget& widget)
{
	bool hasChanged = false;
	auto& value = m_Values[widget.m_VariableName];
	ImGui::PushID(widget.m_ID.c_str());
	switch (value.GetType())
	{
	case CustomInspectorValueType_Int:
		hasChanged = ImGui::DragInt(widget.m_Label.c_str(), &value.m_IntValue, widget.m_FSpeed, static_cast<int32_t>(widget.m_Constratins[0]), static_cast<int32_t>(widget.m_Constratins[1]));
		break;
	case CustomInspectorValueType_Float:
		hasChanged = ImGui::DragFloat(widget.m_Label.c_str(), &value.m_FloatValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_Vector2:
		hasChanged = ImGui::DragFloat2(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_Vector3:
		hasChanged = ImGui::DragFloat3(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_Vector4:
		hasChanged = ImGui::DragFloat4(widget.m_Label.c_str(), value.m_VectorValue, widget.m_FSpeed, widget.m_Constratins[0], widget.m_Constratins[1]);
		break;
	case CustomInspectorValueType_String:
	case CustomInspectorValueType_Bool:
	case CustomInspectorValueType_Texture:
		throw std::exception("Invalid data type for Drag");
	}
	if (widget.m_Tooltip.size() > 0 && ImGui::IsItemHovered()) ImGui::SetTooltip(widget.m_Tooltip.c_str());
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::Button("Reset Value")) value.ResetValue();
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return hasChanged;
}

bool CustomInspector::RenderDropdown(const CustomInspectorWidget& widget)
{
	bool hasChanged = false;
	auto& value = m_Values[widget.m_VariableName];
	ImGui::PushID(widget.m_ID.c_str());
	switch (value.GetType())
	{
	case CustomInspectorValueType_Int: 
		break;
	case CustomInspectorValueType_Float:
	case CustomInspectorValueType_Vector2:
	case CustomInspectorValueType_Vector3:
	case CustomInspectorValueType_Vector4:
	case CustomInspectorValueType_String:
	case CustomInspectorValueType_Bool:
	case CustomInspectorValueType_Texture:
		throw std::exception("Invalid data type for Dropdown");
	}

	if (ImGui::BeginCombo(widget.m_Label.c_str(), widget.m_DropdownOptions[value.m_IntValue].c_str()))
	{
		for (int i = 0; i < static_cast<int32_t>(widget.m_DropdownOptions.size()); i++)
		{
			bool isSelected = (value.m_IntValue == i);
			if (ImGui::Selectable(widget.m_DropdownOptions[i].c_str(), isSelected)) { value.m_IntValue = i; hasChanged = true; }
			if (isSelected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (widget.m_Tooltip.size() > 0 && ImGui::IsItemHovered()) ImGui::SetTooltip(widget.m_Tooltip.c_str());
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::Button("Reset Value")) value.ResetValue();
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return hasChanged;
}
